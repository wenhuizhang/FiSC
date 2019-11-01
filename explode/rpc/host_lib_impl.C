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


#include <sys/types.h>
#include "ekm_lib.h"
#include "rdd/rdd.h"
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "util.h"
// bluntly breaking the abstraction
#include "driver/AbstFS.h"

#include "host_lib_impl.h"

/* This should be defined somewhere else */
#ifndef EKM_PATH
#define EKM_PATH "/dev/ekm"
#endif

using namespace std;

static void load_module(const char* module, const char* args);
static void unload_module(const char* module);
static void wait_dev(const char* dev);

void load_rdd_impl(int disks, int sectors)
{
  char buf[4096];
  int i;
	
  sprintf(buf, "ndisks=%d nsectors=%d", disks, sectors);
  load_module("./rdd.ko", buf);
  for(i = disks-1;i >= 0; i --) {
    sprintf(buf, "/dev/rdd%d", i);
    wait_dev(buf);
  }
}

void unload_rdd_impl(void)
{
  unload_module("rdd");
}

void load_ekm_impl()
{
  load_module("./ekm.ko", "");
  wait_dev("/dev/ekm");
}

void unload_ekm_impl(void)
{
  unload_module("ekm");
}

int open_rdd_impl(const char *devpath) {
  return open(devpath, O_RDONLY);
}

/* done */
int close_rdd_impl(int rdd_fd) {
  return close(rdd_fd);
}

int rdd_get_disk_impl(int rdd_fd, char *disk, int size) {
#ifdef VALGRIND_HACK
  memset(disk, 0, size);
#endif
  return ioctl(rdd_fd, RDD_GET_DISK, disk);
}

int rdd_set_disk_impl(int rdd_fd, char *disk, int size) {
  return ioctl(rdd_fd, RDD_SET_DISK, disk);
}

/* YJF: what is index?? */
int rdd_copy_disk_impl(int rdd_fd, int index) {
  return ioctl(rdd_fd, RDD_COPY_DISK, index);
}

/*
 * start logging on the device associated with handle
 * returns 0 on success and < 0 otherwise
 */
int ekm_start_trace_impl(int ekm_fd, int dev_handle, int want_read) {
  struct ekm_start_trace_request req;
  req.dev = dev_handle;
  req.want_read = want_read;
  return ioctl(ekm_fd, EKM_START_TRACE, &req);
}

/*
 * stop logging on the device associated with handle
 * returns 0 on success and < 0 otherwise
 */
int ekm_stop_trace_impl(int ekm_fd, int dev_handle) {
  printf("stop tracing %d\n", dev_handle);
  return ioctl(ekm_fd, EKM_STOP_TRACE, dev_handle);
}

/*
 * stores the current size of the log in size
 * returns 0 on success and < 0 otherwise
 */
int ekm_get_log_size_impl(int ekm_fd, long *size) {
  return ioctl(ekm_fd, EKM_BUFFER_LOG_SIZE, size);
}

/*
 * given a buffer (req->buffer) of req->size that is large enough to contain
 * the current log, it stores the log in this buffer and sets req->size to the
 * size of the log data now stored in req->buffer
 * returns 0 on success and < 0 otherwise
 */
int ekm_get_buffer_log_impl(int ekm_fd, struct ekm_get_log_request *req) {
#ifdef VALGRIND_HACK
  memset(req->buffer, 0, req->size);
#endif
  return ioctl(ekm_fd, EKM_GET_BUFFER_LOG, req);
}

int ekm_empty_log_impl(int ekm_fd) {
  return ioctl(ekm_fd, EKM_EMPTY_LOG);
}

int ekm_run_impl(int ekm_fd, int pid) {
  return ioctl(ekm_fd, EKM_RUN, pid);
}

int ekm_ran_impl(int ekm_fd, int pid) {
  return ioctl(ekm_fd, EKM_RAN, pid);
}

int ekm_no_run_impl(int ekm_fd, int pid) {
  return ioctl(ekm_fd, EKM_NO_RUN, pid);
}

int ekm_get_pid_impl(int ekm_fd, struct ekm_get_pid_req *s) {
  return ioctl(ekm_fd, EKM_GET_PID, s);
}

int ekm_print_pid_impl(int ekm_fd, int pid_array[]) {
  return ioctl(ekm_fd, EKM_PRINT_PIDS, pid_array);
}

#ifdef CONF_EKM_CHOICE
int ekm_set_choice_impl(int ekm_fd, struct ekm_choice *choice) {
  return ioctl(ekm_fd, EKM_SET_CHOICE, choice);
}

int ekm_get_choice_impl(int ekm_fd, struct ekm_choice *choice) {
#ifdef VALGRIND_HACK
  memset(choice, 0, sizeof *choice);
#endif
  return ioctl(ekm_fd, EKM_GET_CHOICE, choice);
}
#endif

int ekm_kill_disk_impl(int ekm_fd, int dev) {
  return ioctl(ekm_fd, EKM_KILL_DISK, dev);
}

int ekm_revive_disk_impl(int ekm_fd, int dev) {
  return ioctl(ekm_fd, EKM_REVIVE_DISK, dev);
}

#if 0
/* this is systemf for commands that should be executed on the same machine as ekm_net_server. feel free to change the naming. I'll implement this today */

//int track_cmd = 0;

int systemf_impl(const char *fmt, ...)
{
  static char cmd[1024];
  int ret;
  int systemf_return = 0;
  
  va_list ap;
  va_start(ap, fmt);
  vsprintf(cmd, fmt, ap);
  va_end(ap);

  vv_out << "running cmd \"" << cmd << "\"" << endl;

  errno = 0;
  ret = system(cmd);

  if(ret < 0) {
	  perror("system");
	  assert(0);
  }
  systemf_return = WEXITSTATUS(ret);

  if(systemf_return != 0)
	  cout << "nsystemf returns " << systemf_return << endl;
  
  return systemf_return;
}
#endif

int unlink_impl(const char *pathname)
{
    return unlink(pathname);
}

int rmdir_impl(const char *pathname)
{
    return rmdir(pathname);
}

int creat_impl(const char *pathname, mode_t mode)
{
  int fd;

  fd = creat(pathname, mode);
  if(fd < 0)
    perror("Error: %s\n");

  //if(fd < 0)
  return fd;
}

//YJF: this function won't work with RPC
int creat_osync_impl(const char *pathname, mode_t mode)
{
  int fd;

  fd = open(pathname, O_SYNC|O_CREAT, mode);
  if(fd < 0)
    perror("Error: %s\n");
 
  return fd;
}

int mkdir_impl(const char *pathname, mode_t mode)
{
  return mkdir(pathname, mode);
}

int link_impl(const char *oldpath, const char* newpath)
{
  return link(oldpath, newpath);
}

int symlink_impl(const char *oldpath, const char* newpath)
{
  return symlink(oldpath, newpath);
}

int rename_impl(const char *oldpath, const char* newpath)
{
  return rename(oldpath, newpath);
}

int truncate_impl(const char* pathname, off_t length)
{
  return truncate(pathname, (int) length);
}


/* REGULAR FS FUNCTION WRAPPERS */

int open_impl(const char*pathname, int flags)
{
  int fd;

  fd = open(pathname, flags);
    if(fd < 0)
      perror("Error opening device: ");

    return fd;
}

int read_impl(int fd, void *buf, size_t count)
{
  return read(fd, buf, count);
}

int write_impl(int fd, void *buf, size_t count)
{
  return write(fd, buf, count);
}

int close_impl(int fd)
{
  return close(fd);
}

int fsync_impl(int fd)
{
  return fsync(fd);
}

int ftruncate_impl(int fd, off_t length)
{
  return ftruncate(fd, (int) length);
}

int pwrite_impl(int fd, void *buf, size_t count, off_t off)
{
  return pwrite(fd, buf, count, off);
}

void sync_impl()
{
  sync();
}

int stat_impl(const char *pathname, struct stat *buf)
{
  return stat(pathname, buf);
}

void abstract_path_impl(const char *path, AbstFS &fs)
{
  fs.abstract(path);
}

void ekm_app_record_impl(int fd, int type, unsigned len, const char* data)
{
    struct ekm_app_record ar;
    ar.type = type;
    ar.len = len;
    ar.data = (char*)data;

    printf("ekm_app_record(%d, %d, %d, %p)\n", fd, type, len, data);
#if defined(__FreeBSD__)
    // XXX this is broken
    if(ioctl(fd, EKM_APP_RECORD, &ar) < 0) {
#else
    if(ioctl(fd, EKM_APP_RECORD, &ar) < 0) {
#endif
        perror("ioctl EKM_APP_RECORD");
        assert(0);
    }
}


static void load_module(const char* module, const char* args)
{
#if defined(__linux__)
    if(systemf("sudo insmod %s %s", module, args) < 0)
        xi_panic("can't load module ekm!");
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    // FIXME: module args??
    if(systemf("sudo kldload %s %s", module, args) < 0)
        xi_panic("can't load module ekm!");
#else
#    error "os not supported. don't know how to load kernel module!"
#endif
}

static void unload_module(const char* module)
{
#if defined(__linux__)
    if(systemf("sudo rmmod %s", module) < 0)
        xi_panic("can't remove module %s", module);
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    if(systemf("sudo kldunload  %s", module) < 0)
        xi_panic("can't remove module %s", module);
#else
#    error "os not supported. don't know how to load kernel module!"
#endif
}

static void wait_dev(const char* dev)
{
    int trials = 0, fd;

#if defined(__linux__)
    systemf("sudo chown --dereference %s %s",
            getenv("USER"), dev);
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    // no --dereference
    systemf("sudo chown %s %s",
            getenv("USER"), dev);
#else
#    error "os not supported. don't know how to load kernel module!"
#endif
    
    while((fd=open(dev, O_RDONLY))<0 
          && trials++<100) {
        sleep(1);
#if defined(__linux__)
        systemf("sudo chown --dereference %s %s",
                getenv("USER"), dev);
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	// no --dereference
        systemf("sudo chown %s %s",
                getenv("USER"), dev);
#else
#    error "os not supported. don't know how to load kernel module!"
#endif
    }
    
    if(fd < 0) {
        fprintf(stderr, "can't open disk %s!\n", dev);
        assert(0);
    }
    close(fd);
}
