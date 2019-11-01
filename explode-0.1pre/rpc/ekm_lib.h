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


#ifndef __EKM_DEV_LIB_H
#define __EKM_DEV_LIB_H
#include "ekm/ekm.h"

/* file descriptors from init_ekm can be passed to any of the ekm functions,
 *
 * but not to the sbd functions and vice versa
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Initialization functions */
int init_ekm_net(const char *hostname);
int init_ekm_local(void);
int init_rdd_net(int ekm_handle, const char *devpath);
int init_rdd_local(const char *devpath);
int ekm_init_rdd(int ekm_handle, const char *devpath);
int reserve_ekm_handle(void);
void attach_ekm_handle(int handle, const char* hostname);
void detach_ekm_handle(int handle);

/* Deinitialization functions */ 
void close_ekm(int fd);
int close_rdd(int fd);

/* ekm-mod functions */
int ekm_open_device(int fd, const char *name);
int ekm_close_device(int fd, int handle);
int ekm_start_trace(int fd, int handle);
int ekm_stop_trace(int fd, int handle);
#ifdef CONF_EKM_SET_FS
int ekm_set_fs(int fd, int handle, enum fs_e fs);
#endif
int ekm_get_log_size(int fd, long *size);
int ekm_get_buffer_log(int fd, struct ekm_log_request *req);
int ekm_empty_log(int fd);
int ekm_run(int fd, int pid);
int ekm_ran(int fd, int pid);
int ekm_no_run(int fd, int pid);
int ekm_get_pid(int fd, struct ekm_get_pid_req *s);
int ekm_print_pid(int fd, int pid_array[]);
#ifdef CONF_EKM_CHOICE
int ekm_set_choice(int fd, struct ekm_choice *choice);
int ekm_get_choice(int fd, struct ekm_choice *choice);
#endif
int ekm_kill_disk(int fd, int dev);
int ekm_revive_disk(int fd, int dev);


/* sbd functions */
int rdd_get_disk(int fd, char *disk, int size);
int rdd_set_disk(int fd, char *disk, int size);
int rdd_copy_disk(int fd, int index);

  int nsystemf(int s, const char *fmt, ...);

  int ekm_unlink(int ekm_handle, const char *pathname);
  int ekm_rmdir(int ekm_handle, const char *pathname);
  int ekm_creat(int ekm_handle, const char *pathname, mode_t mode);
  int ekm_creat_osync(int ekm_handle, const char *pathname, mode_t mode);
  int ekm_mkdir(int ekm_handle, const char *pathname, mode_t mode);
  int ekm_link(int ekm_handle, const char *oldpath, const char* newpath);
  int ekm_symlink(int ekm_handle, const char *oldpath, const char* newpath);
  int ekm_rename(int ekm_handle, const char *oldpath, const char* newpath);
  int ekm_truncate(int ekm_handle, const char* pathname, off_t length);

  int ekm_open(int ekm_handle, const char*pathname, int flags);
  int ekm_read(int ekm_handle, int fd, void *buf, size_t count);
  int ekm_write(int ekm_handle, int fd, void *buf, size_t count);
  int ekm_close(int ekm_handle, int fd);
  int ekm_fsync(int ekm_handle, int fd);
  int ekm_ftruncate(int ekm_handle, int fs, off_t length);
  int ekm_pwrite(int ekm_handle, int fd, void *buf, size_t count, off_t off);
  void ekm_sync(int ekm_handle);
  int get_fd(int handle);


  void load_ekm();
  void load_rdd(int disks, int sectors);
  void unload_ekm();
  void unload_rdd();
  void ekm_load_rdd(int handle, int disks, int sectors);
  void ekm_unload_rdd(int handle);

  void ekm_app_record(int handle, int type, 
			     unsigned len, const char* data);
	void ekm_app_record(int fd, int type, 
			     unsigned len, const char* data);
#endif
#ifdef __cplusplus
}
#endif

class AbstFS;        
int ekm_abstract_path(int ekm_handle, const char *pathname, AbstFS &fs);

#define RDD_OPEN 1
#define RDD_CLOSE 2
#define NSYSTEMF 3
#define EKM_LIB_OPEN 4
#define EKM_LIB_READ 5
#define EKM_LIB_WRITE 6
#define EKM_LIB_CLOSE 7
#define EKM_LIB_UNLINK 8
#define EKM_LIB_RMDIR 9
#define EKM_LIB_CREAT 10
#define EKM_LIB_MKDIR 11
#define EKM_LIB_TRUNCATE 12
#define EKM_LIB_FSYNC 13
#define EKM_LIB_SYMLINK 14
#define EKM_LIB_RENAME 15
#define EKM_LIB_LINK 16
#define EKM_LIB_ABSTRACT_PATH 17
#define EKM_LIB_FTRUNCATE 18
#define EKM_LIB_PWRITE 19
#define EKM_SYNC 20


#define VALGRIND_HACK

#endif
