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


#ifndef EKM_SYSDEP_H
#define EKM_SYSDEP_H

#ifdef __KERNEL__

//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/percpu.h>
#include <linux/smp_lock.h>
#include <linux/file.h>
#include <linux/quotaops.h>
#include <linux/highmem.h>
#include <linux/module.h>
#include <linux/writeback.h>
#include <linux/hash.h>
#include <linux/suspend.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <asm/bitops.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/jbd.h>
#include <asm/uaccess.h>
#include <linux/explode.h>
#include <linux/file.h>
#include <linux/fs_struct.h>
#include <linux/mount.h>

#define EKM_DECL_LOCK(lock) extern spinlock_t lock
#define EKM_INIT_LOCK(lock) spinlock_t lock = SPIN_LOCK_UNLOCKED
#define EKM_LOCK(lock,flags) spin_lock_irqsave(&(lock), (flags))
#define EKM_UNLOCK(lock,flags) spin_unlock_irqrestore(&(lock), (flags))
#define EKM_GET_PAGE() __get_free_page(GFP_ATOMIC)

#define ENABLE_EKM_RUN
#define ENABLE_EKM_GET_PID

//#define EKM_DEBUG
#ifdef EKM_DEBUG
#define EKM_PRINTK(arg...) printk(arg)
#else
#define EKM_PRINTK(arg...)
#endif

#ifdef CONFIG_KGDB
#    include <linux/kgdb.h>
#else
#    define breakpoint()
#endif

#endif // #ifdef __KERNEL__

/* YJF: XXXX use 255 as a magic number to indicate md0 */
#define MD_MAGIC (255)

#endif
