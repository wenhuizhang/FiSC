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

#include "HostLib.h"
#include "host_lib_impl.h"
#include "mcl.h"

using namespace std;
void HostLib::init() {
  ekm_fd = open(EKM_PATH, O_RDONLY);

  if(ekm_fd < 0)
    exit(-1);
}

HostLib::~HostLib() {
}

/* loading functions */
void HostLib::load_rdd(int disks, int sectors) {
  load_rdd_impl(disks, sectors);
}

void HostLib::unload_rdd() {
  unload_rdd_impl();
}

void HostLib::load_ekm() {
  load_ekm_impl();
  init();
}

void HostLib::unload_ekm() {
  if(ekm_fd != -1)
    assert(!close(ekm_fd));
  unload_ekm_impl();
}

/* rdd functions */
int HostLib::open_rdd(const char *devpath) {
  return open_rdd_impl(devpath);
}

int HostLib::close_rdd(int fd) {
  return close_rdd_impl(fd);
}

int HostLib::rdd_get_disk(int fd, char *disk, int size) {
  return rdd_get_disk_impl(fd, disk, size);
}

int HostLib::rdd_set_disk(int fd, char *disk, int size) {
  return rdd_set_disk_impl(fd, disk, size);
}

int HostLib::rdd_copy_disk(int fd, int index) {
  return rdd_copy_disk_impl(fd, index);
}

/* ekm-mod functions */
int HostLib::ekm_start_trace(int dev, int want_read) {
  return ekm_start_trace_impl(ekm_fd, dev, want_read);
}

int HostLib::ekm_stop_trace(int dev) {
  return ekm_stop_trace_impl(ekm_fd, dev);
}

int HostLib::ekm_get_log_size(long *size) {
  return ekm_get_log_size_impl(ekm_fd, size);
}

int HostLib::ekm_get_buffer_log(struct ekm_get_log_request *req) {
  return ekm_get_buffer_log_impl(ekm_fd, req);
}

int HostLib::ekm_empty_log() {
  return ekm_empty_log_impl(ekm_fd);
}

int HostLib::ekm_run(int pid) {
  return ekm_run_impl(ekm_fd, pid);
}

int HostLib::ekm_ran(int pid) {
  return ekm_ran_impl(ekm_fd, pid);
}

int HostLib::ekm_no_run(int pid) {
  return ekm_no_run_impl(ekm_fd, pid);
}

int HostLib::ekm_get_pid(struct ekm_get_pid_req *s) {
  return ekm_get_pid_impl(ekm_fd, s);
}

int HostLib::ekm_print_pid(int pid_array[]) {
  return ekm_print_pid_impl(ekm_fd, pid_array);
}

#ifdef CONF_EKM_CHOICE
int HostLib::ekm_set_choice(struct ekm_choice *choice) {
  return ekm_set_choice_impl(ekm_fd, choice);
}

int HostLib::ekm_get_choice(struct ekm_choice *choice) {
  return ekm_get_choice_impl(ekm_fd, choice);
}
#endif

int HostLib::ekm_kill_disk(int dev) {
  return ekm_kill_disk_impl(ekm_fd, dev);
}

int HostLib::ekm_revive_disk(int dev) {
  return ekm_revive_disk_impl(ekm_fd, dev);
}

int HostLib::systemf(const char *fmt, ...) {
  /* this has to be here because varargs suck and you can't pass them simply */
  static char cmd[1024];
  int ret;
  int systemf_return = 0;
  
  va_list ap;
  va_start(ap, fmt);
  vsprintf(cmd, fmt, ap);
  va_end(ap);

  vv_out << "running cmd \"" << cmd << "\"" << "\n";

  errno = 0;
  ret = system(cmd);

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
int HostLib::unlink(const char *pathname) {
  return unlink_impl(pathname);
}

int HostLib::rmdir(const char *pathname) {
  return rmdir_impl(pathname);
}

int HostLib::creat(const char *pathname, mode_t mode) {
  return creat_impl(pathname, mode);
}

int HostLib::creat_osync(const char *pathname, mode_t mode) {
  return creat_osync_impl(pathname, mode);
}

int HostLib::mkdir(const char *pathname, mode_t mode) {
  return mkdir_impl(pathname, mode);
}

int HostLib::link(const char *oldpath, const char* newpath) {
  return link_impl(oldpath, newpath);
}

int HostLib::symlink(const char *oldpath, const char* newpath) {
  return symlink_impl(oldpath, newpath);
}

int HostLib::rename(const char *oldpath, const char* newpath) {
  return rename_impl(oldpath, newpath);
}

int HostLib::truncate(const char* pathname, off_t length) {
  return truncate_impl(pathname, length);
}

/*fs functions with fd */
int HostLib::open(const char*pathname, int flags) {
  return open_impl(pathname, flags);
}

int HostLib::read(int fd, void *buf, size_t count) {
  return read_impl(fd, buf, count);
}

int HostLib::write(int fd, void *buf, size_t count) {
  return write_impl(fd, buf, count);
}

int HostLib::close(int fd) {
  return close_impl(fd);
}

int HostLib::fsync(int fd) {
  return fsync_impl(fd);
}

int HostLib::ftruncate(int fd, off_t length) {
  return ftruncate_impl(fd, length);
}

int HostLib::pwrite(int fd, void *buf, size_t count, off_t off){
  return pwrite_impl(fd, buf, count, off);
}

void HostLib::sync() {
  return sync_impl();
}

int HostLib::stat(const char *pathname, struct stat *buf) {
  return stat_impl(pathname, buf);
}

void HostLib::abstract_path(const char *pathname, AbstFS &fs) {
  abstract_path_impl(pathname, fs);
}

void HostLib::ekm_app_record(int type, unsigned len, const char* data) {
  ekm_app_record_impl(ekm_fd, type, len,data);
}
