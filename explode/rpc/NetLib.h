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


#ifndef __NET_LIB_H
#define __NET_LIB_H

#include "HostLib.h"
#include "util.h"
#include "ekm/ekm.h"
#include "explode_rpc.h"

class AbstFS;
class NetLib : public HostLib {
 private:
  CLIENT *clnt;
  const char *_host;

 public:
  NetLib(const char* h) : clnt(NULL) {_host = xstrdup(h);}
  void init(void);
  ~NetLib();

  const char* host(void) {return _host;}

  int systemf(const char *fmt, ...);

  /* fs functions without fd */
  int unlink(const char *pathname);
  int rmdir(const char *pathname);
  int creat(const char *pathname, mode_t mode);
  int creat_osync(const char *pathname, mode_t mode);
  int mkdir(const char *pathname, mode_t mode);
  int link(const char *oldpath, const char* newpath);
  int symlink(const char *oldpath, const char* newpath);
  int rename(const char *oldpath, const char* newpath);
  int truncate(const char* pathname, off_t length);

  /*fs functions with fd */
  int open(const char*pathname, int flags);
  int read(int fd, void *buf, size_t count);
  int write(int fd, void *buf, size_t count);
  int close(int fd);
  int fsync(int fd);
  int ftruncate(int fd, off_t length);
  int pwrite(int fd, void *buf, size_t count, off_t off);
  void sync();
  int stat(const char *pathname, struct stat *buf);
  void abstract_path(const char *pathname, AbstFS &fs);
#if 0
  void ekm_app_record(int type, unsigned len, const char* data);
#endif
};

#endif
