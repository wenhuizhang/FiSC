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


#ifndef __EKM_LOG_BUF_H
#define __EKM_LOG_BUF_H

#include "../ekm/ekm_sysdep.h"

#ifdef __KERNEL__

EKM_DECL_LOCK(log_lock);

extern int ekm_lsn;

long get_log_size(void);
int copy_log_buffer(char *buffer, long *size);
void empty_log(void);
void log_add_buf(const char *data, long data_bytes);

/* FIXME use this to detect the koops triggered by
   bh->b_data == 0x400.  Why is this happening though?
   
   well, this happens if bh->b_data lives in high mem.  we need to
   use a bounce buffer.  anyway, the simplest hack is just to
   configure the kernel not to use high mem
*/
#define INVALID(p) ((void*)p <= (void*)0x1000)


// make them explicit, so we know exactly how to unmarshal
static inline void log_add_int(int num)
{
	log_add_buf((char*)&num, sizeof num);
}
static inline void log_add_int8(uint64_t num)
{
	log_add_buf((char*)&num, sizeof num);
}
static inline void log_add_long(long num)
{
	log_add_buf((char*)&num, sizeof num);
}

static inline void log_add_str(const char* s)
{
	int len = strlen(s);
	log_add_int(len);
	log_add_buf(s, len);
}

static inline void log_add_sized_buf(const char *buf, int len)
{
        if(len > 0) {
                log_add_int(len);
                log_add_buf(buf, len);
        } else
                log_add_int(0);
}

#endif //#ifdef __KERNEL__

#endif
