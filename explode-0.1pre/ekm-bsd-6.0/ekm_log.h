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

//#include <sys/buf.h>
struct buf;
struct bio;
struct g_geom;
#include "ekm_log_generated.h"

#define EKM_LOG_MAGIC (0xF12C0109)

//struct mtx log_mutex;

/* log record types */
#define EKM_CAT_SHIFT (16)
#define EKM_CAT_MASK       ((-1)<<EKM_CAT_SHIFT)
#define EKM_CAT(t)         (((t) & EKM_CAT_MASK)>>EKM_CAT_SHIFT)
#define EKM_MINOR(t)       ((t)&((1<<EKM_CAT_SHIFT)-1))
#define EKM_MAKE_TYPE(cat, minor) \
			   (((cat)<<EKM_CAT_SHIFT) | minor)

#define EKM_BH_CAT         (1)
#define EKM_BIO_CAT        (2)
#define EKM_REQ_CAT        (3)
#define EKM_SYSCALL_CAT    (4)

#define EKM_USR_CAT_BEGIN  (16)
#define EKM_USR_CAT_END    (0xEF)

#define EKM_LOG_BH_P(rec)  (EKM_CAT((rec)->type)==EKM_BH_CAT)
#define EKM_LOG_BIO_P(rec) (EKM_CAT((rec)->type)==EKM_BIO_CAT)
#define EKM_LOG_SYSCALL_P(rec) \
			   (EKM_CAT((rec)->type)==EKM_SYSCALL_CAT)

#define EKM_LOG_USR_P(rec) \
                           (EKM_CAT((rec)->type)>=EKM_USR_CAT_BEGIN \
			    && EKM_CAT((rec)->type)<=EKM_USR_CAT_END)

#define EKM_REC_TYPE_DATA_P(type) \
	  (type == EKM_BH_dirty \
	    || type == EKM_BH_write \
	    || type == EKM_BH_write_sync \
	    || type == EKM_BIO_start_write \
	    || type == EKM_BIO_write_done \
            || type == EKM_BIO_read)

#define EKM_REC_DATA_P(rec) EKM_REC_TYPE_DATA_P((rec)->type)

#define EKM_LOG_MAGIC (0xF12C0109)
#define EKM_FL_COMMIT (1<<0)

/* log record header */
struct ekm_record_hdr {
	int size;  /* size of the log record */
	int lsn; /* unique, monotonic increasing sequence number */
	int type; /* record type */
	int pid;
	int magic;
};

/* type == EKM_BH_* */
struct ekm_bh_record {
	struct ekm_record_hdr hdr;
	int br_device; /* device where this block buffer is written to */
	int br_flags;  /* for blocks containing commit record,
			  EKM_FL_COMMIT must be set.  For JFS and
			  XFS, treat all log writes as commit
			  records.  */
	long br_block;
	int br_size;
	char br_data[0];
};

/* type == EKM_BIO_* */
struct ekm_bio_record {
	struct ekm_record_hdr hdr;
	int bir_device; /* device where this sector buffer is written to */
	int bir_flags;  /* for blocks containing commit record,
			   EKM_FL_COMMIT must be set.  For JFS and
			   XFS, treat all log writes as commit
			   records.  */
	long bir_sector;
	int bir_size;
	char bir_data[0];
};

/* type == EKM_APP_* */
struct ekm_generic_record {
	struct ekm_record_hdr hdr;
	char gr_data[0];
};

/* YJF: returns the lenth of a record based on its type */
#define EKM_REC_LEN(rec) (((struct ekm_record_hdr*)rec)->size)

int start_trace(int dev);
int stop_trace(int dev);
void stop_traces(void);
long get_log_size(void);
int copy_log_buffer(char *buffer, long *size);
void empty_log(void);
void ekm_log_app_record(struct ekm_generic_record *gr);
void ekm_log_install(void);
void ekm_log_uninstall(void);


int is_traced_udev(int dev);
int is_traced_dev(struct buf *bh);
int is_traced_geom(struct g_geom *gp);

void log_add_bh_entry(struct buf *bh, int type);
void log_add_req_entry(struct bio *bio, int type);



#endif
