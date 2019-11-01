/* -*- Mode: C++; c-basic-offset: 2 -*- */

/*
 *
 * Copyright (C) 2008 Can Sar (csar@cs.stanford.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */


#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>

#include <iostream>

#include "NetLib.h"
#include "mcl.h"
#include "explode_rpc.h"
// bluntly breaking the abstraction
#include "driver/AbstFS.h"


using namespace std;
void NetLib::init(void) {
  int it = 25;
  for(int i = 0; i < it; i++) {
    clnt = clnt_create (_host, EXPLODEPROG, EXPLODEVERS, "tcp");
    if (clnt == NULL) {
      clnt_pcreateerror (_host);
      sleep(3);
    }
    else
      break;
  }
  if(!clnt)
    xi_panic("can't create RPC client\n");
}

NetLib::~NetLib() {
  clnt_destroy (clnt);
  delete _host;
}


int NetLib::systemf(const char *fmt, ...) {
  /* this has to be here because varargs suck and you can't pass them simply */
  char *cmd = (char *)malloc(1024);
  int ret;
  int *result_1;
  int systemf_return = 0;

  assert(cmd);

  va_list ap;
  va_start(ap, fmt);
  vsprintf(cmd, fmt, ap);
  va_end(ap);

  vv_out << "running cmd \"" << cmd << "\"" << "\n";

  errno = 0;
  //ret = system(cmd);

  result_1 = rpc_systemf_1(cmd, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }
  free(cmd);
  ret = *result_1;

  if(ret < 0) {
    perror("system");
    assert(0);
  }
  systemf_return = WEXITSTATUS(ret);

  if(systemf_return != 0)
    printf("nsystemf returns %d\n", systemf_return);
  
  return systemf_return;
}

/* fs functions without fd */
int NetLib::unlink(const char *pathname) {
  int  *result_1;

  result_1 = rpc_unlink_1((char *) pathname, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::rmdir(const char *pathname) {
  int *result_1;

  result_1 = rpc_rmdir_1((char *) pathname, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::creat(const char *pathname, mode_t mode) {
  int *result_1;

  result_1 = rpc_creat_1((char *) pathname, mode, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::creat_osync(const char *pathname, mode_t mode) {
  int *result_1;

  result_1 = rpc_creat_osync_1((char *) pathname, mode, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::mkdir(const char *pathname, mode_t mode) {
  int *result_1;

  result_1 = rpc_mkdir_1((char *) pathname, mode, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::link(const char *oldpath, const char* newpath) {
  int *result_1;

  result_1 = rpc_link_1((char *) oldpath, (char *) newpath, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::symlink(const char *oldpath, const char* newpath) {
  int *result_1;

  result_1 = rpc_symlink_1((char *) oldpath, (char *) newpath, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::rename(const char *oldpath, const char* newpath) {
  int *result_1;

  result_1 = rpc_rename_1((char *) oldpath, (char *) newpath, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::truncate(const char* pathname, off_t length) {
  int *result_1;

  result_1 = rpc_truncate_1((char *) pathname, length, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

/*fs functions with fd */
int NetLib::open(const char*pathname, int flags) {
  int *result_1;

  result_1 = rpc_open_1((char *) pathname, flags, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::read(int fd, void *buf, size_t count) {
  read_res  *result_1;
  result_1 = rpc_read_1(fd, count, clnt);
  if (result_1 == (read_res *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  if(result_1->errornum == 0) {
    memcpy(buf, result_1->read_res_u.buf.buffer_val,
           result_1->read_res_u.buf.buffer_len); 
    return result_1->read_res_u.buf.buffer_len;
  }
	
  return result_1->errornum;
}

int NetLib::write(int fd, void *buf, size_t count) {
  buffer rpc_buf;
  int *result_1;

  rpc_buf.buffer_val = (char *) malloc(count);

  memcpy(rpc_buf.buffer_val, buf, count);

  rpc_buf.buffer_len = count;

  result_1 = rpc_write_1(fd, rpc_buf, count, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}


int NetLib::close(int fd) {
  int *result_1;

  result_1 = rpc_close_1(fd, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::fsync(int fd) {
  int *result_1;

  result_1 = rpc_fsync_1(fd, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::ftruncate(int fd, off_t length) {
  int *result_1;

  result_1 = rpc_ftruncate_1(fd, length, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

int NetLib::pwrite(int fd, void *buf, size_t count, off_t off){
  buffer rpc_buf;
  int *result_1;

  rpc_buf.buffer_val = (char *) malloc(count);

  memcpy(rpc_buf.buffer_val, buf, count);

  rpc_buf.buffer_len = count;

  result_1 = rpc_pwrite_1(fd, rpc_buf, count, off, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  return *result_1;
}

void NetLib::sync() {
  int *result_1;

  result_1 = rpc_sync_1(clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }
}

int NetLib::stat(const char *pathname, struct stat *buf){
  stat_res  *result_1;
  assert(sizeof(*buf) == 96);

  result_1 = rpc_stat_1((char *)pathname, clnt);
  if (result_1 == (stat_res *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  if(result_1->errornum == 0) {
    memcpy(buf, result_1->stat_res_u.buf, sizeof(*buf)); 
    return 0;
  }
	
  return -1;
}

void NetLib::abstract_path(const char *pathname, AbstFS &fs) {
  abstract_path_res  *result_1;

  result_1 = rpc_abstract_path_1((char *) pathname, clnt);
  if (result_1 == (abstract_path_res *) NULL) {
    clnt_perror (clnt, "call failed");
    assert(0);
  }

  if(result_1->errornum == 0) {
    fs.unmarshal(result_1->abstract_path_res_u.fs_buf.fs_buf_val,
                 result_1->abstract_path_res_u.fs_buf.fs_buf_len);
    fs.remove_ignored();
  }

}

#if 0
void HostLib::ekm_app_record(int type, unsigned len, const char* data) {
  ekm_app_record_impl(ekm_fd, type, len,data);
}
#endif
