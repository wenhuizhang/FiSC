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


#ifdef __cplusplus
extern "C" {
#endif
  void load_rdd_impl(int disks, int sectors);
  void unload_rdd_impl(void);
  void load_ekm_impl();
  void unload_ekm_impl(void);

  /* rdd functions */
  int open_rdd_impl(const char *devpath);
  int close_rdd_impl(int rdd_fd);
  int rdd_get_disk_impl(int rdd_fd, char *disk, int size);
  int rdd_set_disk_impl(int rdd_fd, char *disk, int size);
  int rdd_copy_disk_impl(int rdd_fd, int index);

  /* ekm-mod functions */
  int ekm_start_trace_impl(int ekm_fd, int dev_handle, int want_read);
  int ekm_stop_trace_impl(int ekm_fd, int dev_handle);
  int ekm_get_log_size_impl(int ekm_fd, long *size);
  int ekm_get_buffer_log_impl(int ekm_fd, struct ekm_get_log_request *req);
  int ekm_empty_log_impl(int ekm_fd);
  int ekm_run_impl(int ekm_fd, int pid);
  int ekm_ran_impl(int ekm_fd, int pid);
  int ekm_no_run_impl(int ekm_fd, int pid);
  int ekm_get_pid_impl(int ekm_fd, struct ekm_get_pid_req *s);
  int ekm_print_pid_impl(int ekm_fd, int pid_array[]);
#ifdef CONF_EKM_CHOICE
  int ekm_set_choice_impl(int ekm_fd, struct ekm_choice *choice);
  int ekm_get_choice_impl(int ekm_fd, struct ekm_choice *choice);
#endif
  int ekm_kill_disk_impl(int ekm_fd, int dev);
  int ekm_revive_disk_impl(int ekm_fd, int dev);

  int systemf_impl(const char *fmt, ...);

  /* fs functions without fd */
  int unlink_impl(const char *pathname);
  int rmdir_impl(const char *pathname);
  int creat_impl(const char *pathname, mode_t mode);
  int creat_osync_impl(const char *pathname, mode_t mode);
  int mkdir_impl(const char *pathname, mode_t mode);
  int link_impl(const char *oldpath, const char* newpath);
  int symlink_impl(const char *oldpath, const char* newpath);
  int rename_impl(const char *oldpath, const char* newpath);
  int truncate_impl(const char* pathname, off_t length);

  /* fs functions with fd */
  int open_impl(const char*pathname, int flags);
  int read_impl(int fd, void *buf, size_t count);
  int write_impl(int fd, void *buf, size_t count);
  int close_impl(int fd);
  int fsync_impl(int fd);
  int ftruncate_impl(int fd, off_t length);
  int pwrite_impl(int fd, void *buf, size_t count, off_t off);
  void sync_impl();
  void abstract_path_impl(const char *path, AbstFS &fs);
  int stat_impl(const char *pathname, struct stat *buf);
  void ekm_app_record_impl(int fd, int type, unsigned len, const char* data);
#ifdef __cplusplus
}
#endif

