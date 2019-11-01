/*
 *
 * Copyright (C) 2008 Can Sar (csar@cs.stanford.edu) and
 *                    Junfeng Yang (junfeng@cs.columbia.edu)
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


#ifndef __EKM_LOG_H
#define __EKM_LOG_H

#include "../crash/ekm_log_types.h"

#define EKM_LINUX_CAT             (16) /* EKM_RESERVED_CAT_BEGIN */
#define EKM_BH_dirty           ( 1|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BH_clean           ( 2|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BH_lock    	       ( 3|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BH_wait            ( 4|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BH_unlock          ( 5|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BH_write           ( 6|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BH_write_sync      ( 7|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))

#define EKM_BIO_start_write    ( 8|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BIO_wait           ( 9|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BIO_write          (10|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BIO_read           (11|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))
#define EKM_BIO_write_done     (12|(EKM_LINUX_CAT<<EKM_CAT_SHIFT))

#define EKM_REC_TYPE_HAS_DATA(type) \
	  (type == EKM_BH_dirty \
	    || type == EKM_BH_write \
	    || type == EKM_BH_write_sync \
	    || type == EKM_BIO_read \
	    || type == EKM_BIO_start_write \
	    || type == EKM_BIO_write_done)

#define EKM_REC_DATA_P(rec) EKM_REC_TYPE_DATA_P((rec)->type)

/* 
   log record header
    int type;  record type
    int lsn;   unique, monotonic increasing sequence number
    int pid;   id of the kernel thread who wrote this record
    int magic; must match EKM_LOG_MAGIC 
*/

/* 
   EKM_BH_* and EKM_BIO_* log format
   log record header
   int br_device; device where this block buffer is written to
   int br_flags;  for blocks containing commit record,
                  EKM_FL_COMMIT must be set.  For JFS and
                  XFS, treat all log writes as commit
                  records. 
   long br_sector;
   int br_size;
   char br_data[0]; // size of this buffer is br_size
*/


/* EKM_APP_* log request.  this is *not* the format of how an app
   record is stored in ekm log */
struct ekm_app_record {
        int type;
        unsigned len;
        char* data;
};

#ifdef __KERNEL__
// kernel portion
#define EKM_LOG_DATA_SIZE \
	(PAGE_SIZE - sizeof(struct ekm_log *) - sizeof(long))
typedef struct ekm_log {
        struct ekm_log *next;
        long size;
        char data[EKM_LOG_DATA_SIZE];
} ekm_log;

struct ekm_start_trace_request;
int start_trace(struct ekm_start_trace_request *req);
int stop_trace(int handle);
void stop_traces(void);
void ekm_log_app_record(struct ekm_app_record *gr);
void ekm_log_install(void);
void ekm_log_uninstall(void);

#else  // #ifdef __KERNEL__
#	undef __user
#	define __user

/* application defined log record types.  users have to write code
   to manipulate these records.  won't be generated by
   gen_ekm_log */
#endif   // #ifdef __KERNEL__

#endif //#ifndef __EKM_LOG_H
