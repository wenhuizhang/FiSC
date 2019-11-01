/*
 *
 * Copyright (C) 2008 Junfeng Yang (junfeng@cs.columbia.edu)
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


#ifndef __BISIM_H
#define __BISIM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif
	int bi_creat(const char *pathname, mode_t mode);
	int bi_link(const char *oldpath, const char* newpath);
	int bi_rename(const char *oldpath, const char *newpath);
	int bi_unlink (const char *pathname);
	int bi_symlink(const char *oldpath, const char *newpath);

	int bi_mkdir(const char *pathname, mode_t mode);
	int bi_rmdir(const char *pathname);
	int bi_truncate(const char* pathname, off_t length);
	int bi_test_write(const char *pathname, unsigned i);
	int bi_test_truncate(const char* pathname);
#if 0
	int bi_truncate(const char *path, off_t length);
	int bi_read(const char *pathname, void* buf, size_t size);
	int bi_write(const char *pathname, const void *buf, size_t size);
	int bi_readlink(const char *path, char *buf, size_t bufsiz);

	int bi_chmod(const char *path, mode_t mode);
	int bi_chown(const char *path, uid_t owner, gid_t group);
	int bi_fchown(int fd, uid_t owner, gid_t group);
	int bi_lchown(const char *path, uid_t owner, gid_t group);
	int bi_stat(const char *file_name, struct stat *buf);
	int bi_fstat(int filedes, struct stat *buf);
	int bi_lstat(const char *file_name, struct stat *buf);
	int bi_ioctl(int d, int request, ...);
	int bi_mount(const char *source, const char *target, 
		     const char *filesystemtype, 
		     unsigned long mountflags, const void *data);

	int bi_umount(const char *target);
	int bi_umount2(const char *target, int flags);
#endif
#ifdef __cplusplus
}
#endif

#endif
