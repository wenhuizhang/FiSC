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


#ifndef __HOST_LIB_H
#define __HOST_LIB_H

#include "ekm/ekm.h"
#include "config.h"

#define EKM_PATH "/dev/ekm"

enum {
  NO_READ = 0,
  WANT_READ = 1, 
};

class AbstFS;
class HostLib {
private:
  int ekm_fd;

public:
  HostLib() : ekm_fd(-1) { };
  virtual void init();
  //virtual int init(const char *hostname) { assert(0); }
  ~HostLib();

  /* loading functions */
  virtual void load_rdd(int disks, int sectors);
  virtual void unload_rdd();
  virtual void load_ekm();
  virtual void unload_ekm();

  /* rdd functions */
  virtual int open_rdd(const char *devpath);
  virtual int close_rdd(int fd);
  virtual int rdd_get_disk(int fd, char *disk, int size);
  virtual int rdd_set_disk(int fd, char *disk, int size);
  virtual int rdd_copy_disk(int fd, int index);

  /* ekm-mod functions */
  virtual int ekm_start_trace(int dev, int want_read);
  virtual int ekm_stop_trace(int dev);
  virtual int ekm_get_log_size(long *size);
  virtual int ekm_get_buffer_log(struct ekm_get_log_request *req);
  virtual int ekm_empty_log();
  virtual int ekm_run(int pid);
  virtual int ekm_ran(int pid);
  virtual int ekm_no_run(int pid);
  virtual int ekm_get_pid(struct ekm_get_pid_req *s);
  virtual int ekm_print_pid(int pid_array[]);
#ifdef CONF_EKM_CHOICE
  virtual int ekm_set_choice(struct ekm_choice *choice);
  virtual int ekm_get_choice(struct ekm_choice *choice);
#endif
  virtual int ekm_kill_disk(int dev);
  virtual int ekm_revive_disk(int dev);

  virtual int systemf(const char *fmt, ...);

  /* fs functions without fd */
  virtual int unlink(const char *pathname);
  virtual int rmdir(const char *pathname);
  virtual int creat(const char *pathname, mode_t mode);
  virtual int creat_osync(const char *pathname, mode_t mode);
  virtual int mkdir(const char *pathname, mode_t mode);
  virtual int link(const char *oldpath, const char* newpath);
  virtual int symlink(const char *oldpath, const char* newpath);
  virtual int rename(const char *oldpath, const char* newpath);
  virtual int truncate(const char* pathname, off_t length);

  /*fs functions with fd */
  virtual int open(const char*pathname, int flags);
  virtual int read(int fd, void *buf, size_t count);
  virtual int write(int fd, void *buf, size_t count);
  virtual int close(int fd);
  virtual int fsync(int fd);
  virtual int ftruncate(int fd, off_t length);
  virtual int pwrite(int fd, void *buf, size_t count, off_t off);
  virtual void sync();
  virtual int stat(const char *pathname, struct stat *buf);
  virtual void abstract_path(const char *pathname, AbstFS &fs);

  virtual void ekm_app_record(int type, unsigned len, const char* data);
};

#endif
