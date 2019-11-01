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


#ifndef __EKM_IOCTL_H
#define __EKM_IOCTL_H

/* this shouldn't conflict with anything listed in
   Documentation/ioctl-number.txt */
#define EKM_IOC_MAGIC (0xD1)

/*
 * IOW actually means reading from the user (this is from the user's
 * perspective) via copy_from_user while IOW means doing copy_to_user
 */
#define EKM_OPEN_DEVICE _IOW(EKM_IOC_MAGIC, 1, char)
#define EKM_CLOSE_DEVICE _IO(EKM_IOC_MAGIC, 2)
#define EKM_START_TRACE _IOW(EKM_IOC_MAGIC, 3, struct ekm_start_trace_request)
#define EKM_STOP_TRACE _IO(EKM_IOC_MAGIC, 4)
#define EKM_SET_FS _IOW(EKM_IOC_MAGIC, 5, struct ekm_setfs_request)
#define EKM_BUFFER_LOG_SIZE _IO(EKM_IOC_MAGIC, 6)
#define EKM_GET_BUFFER_LOG _IOWR(EKM_IOC_MAGIC, 7, struct ekm_get_log_request)
#define EKM_EMPTY_LOG _IO(EKM_IOC_MAGIC, 8)
#define EKM_RUN _IO(EKM_IOC_MAGIC, 9)
#define EKM_RAN _IO(EKM_IOC_MAGIC, 10)
#define EKM_NO_RUN _IO(EKM_IOC_MAGIC, 11)
#define EKM_GET_PID _IOW(EKM_IOC_MAGIC, 12, struct ekm_get_pid_req)
#define EKM_PRINT_PIDS _IOW(EKM_IOC_MAGIC, 13, int)
#define EKM_SET_CHOICE _IOW(EKM_IOC_MAGIC, 14, struct ekm_choice)
#define EKM_GET_CHOICE _IOR(EKM_IOC_MAGIC, 15, struct ekm_choice)

#define EKM_LOAD_RDD _IO(EKM_IOC_MAGIC, 16)
#define EKM_UNLOAD_RDD _IO(EKM_IOC_MAGIC, 17)

#define EKM_APP_RECORD _IOW(EKM_IOC_MAGIC, 18, struct ekm_app_record)

#define EKM_KILL_DISK _IOW(EKM_IOC_MAGIC, 19, unsigned)
#define EKM_REVIVE_DISK _IOW(EKM_IOC_MAGIC, 20, unsigned)

struct ekm_get_log_request {
  long size;
  char *buffer;
};

struct ekm_start_trace_request {
  int dev;
  int want_read;
};

/* YJF: only handle 1 thread id for now */
struct ekm_get_pid_req{
  int pid; /* output to user space */
  char path[4096]; /* input to kernel */
};

#endif
