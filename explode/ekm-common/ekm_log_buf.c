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


#include "ekm_log_buf.h"

#define EKM_LOG_DATA_SIZE \
	(PAGE_SIZE - sizeof(struct ekm_log *) - sizeof(long))

typedef struct ekm_log {
  struct ekm_log *next;
  long size;
  char data[EKM_LOG_DATA_SIZE];
} ekm_log;

/* this is to synchronize access to the main log */
EKM_INIT_LOCK(log_lock);

static ekm_log *ekm_log_start = NULL;
static ekm_log *ekm_log_cur = NULL;
static long ekm_log_size = 0;
int ekm_lsn = 0;

static int ekm_num_pages = 0;

static void empty_log_helper(ekm_log *log);

static int add_log_page(void) {
        ekm_log *next;
        disable_choose();
        next = (ekm_log *) EKM_GET_PAGE();
        enable_choose();
        if (next == NULL) {
                printk("EKM error: can't get memory for log\n");
                // YJF: just die
                BUG();
                return 0;
        }

        if(ekm_num_pages > 1000)
                EKM_PRINTK("EKM: %d > 1000 pages\n", ekm_num_pages);

        //printk("EKM: add log page %p, total = %d\n", next, ekm_num_pages);
        ekm_num_pages ++;
        if (ekm_log_start == NULL)
                ekm_log_start = next;
        else
                ekm_log_cur->next = next;
        ekm_log_cur = next;
        ekm_log_cur->next = NULL;
        ekm_log_cur->size = 0;
        return 1;
}

void log_add_buf(const char *data, long data_bytes) {
        /* Until we figure out why we're getting bad pointers passed
           in, we might as well avoid dereferencing them. */
        /* The above is the funniest comment I've read in a while :-) - Can */
        if (INVALID(data)) {
                printk("EKM error: can't log data from invalid pointer %p\n", 
                       data);
                return;
        }
  
        while (data_bytes > 0) {
                int bytes_available;
                int copy_bytes;

                if (ekm_log_cur == NULL 
                    || ekm_log_cur->size >= EKM_LOG_DATA_SIZE) {
                        if (!add_log_page())
                                return;
                }

                bytes_available = EKM_LOG_DATA_SIZE - ekm_log_cur->size;
                copy_bytes = data_bytes;
                if (copy_bytes > bytes_available)
                        copy_bytes = bytes_available;
    
                memcpy(ekm_log_cur->data + ekm_log_cur->size, data, copy_bytes);
                ekm_log_cur->size += copy_bytes;
                data += copy_bytes;
                data_bytes -= copy_bytes;
                /* YJF:  need to update this size as well */
                ekm_log_size += copy_bytes;
        }

        ekm_log_size += data_bytes;
}

long get_log_size(void)
{
        long temp;
        unsigned long flags;
        //spin_lock_irqsave(&log_lock, flags);
        EKM_LOCK(log_lock, flags);
        temp = ekm_log_size;
        EKM_UNLOCK(log_lock, flags);
        return temp;
}

int copy_log_buffer(char __user * buffer, long *size) {
        ekm_log *temp_start;
        ekm_log *temp_cur;
        long copied = 0;
        unsigned long flags;

        EKM_LOCK(log_lock, flags);
        temp_start = ekm_log_start;

        if(ekm_log_size > *size) {
                EKM_UNLOCK(log_lock, flags);
                return -EOVERFLOW;
        }
        temp_cur = temp_start = ekm_log_start;
        ekm_log_start = ekm_log_cur = NULL;
        ekm_log_size = 0;
        EKM_UNLOCK(log_lock, flags);

        while(temp_cur != NULL) {
                if(copy_to_user(buffer + copied, 
                                temp_cur->data, temp_cur->size)) {
                        empty_log_helper(temp_start);
                        return -EFAULT;
                }
                copied += temp_cur->size;
                temp_cur = temp_cur->next;
        }
        *size = copied;
        empty_log_helper(temp_start);

        return 0;
}

void empty_log(void) {
        unsigned long flags;
        EKM_LOCK(log_lock, flags);

        empty_log_helper(ekm_log_start);

        ekm_log_start = NULL;
        ekm_log_cur = NULL;
        ekm_log_size = 0;
        ekm_lsn = 0;

        EKM_UNLOCK(log_lock, flags);
}

/* this must be called with the lock held */
static void empty_log_helper(ekm_log *log) {
        ekm_log *temp;

        while(log != NULL) {
                temp = log->next;
                free_page((unsigned long) log);
                ekm_num_pages --;
                // printk("EKM: log free page %p, total = %d\n", 
                // log, ekm_num_pages);
                log = temp;
        }
}
