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


#ifndef __EKM_LOG_TYPES_H
#define __EKM_LOG_TYPES_H

/* upper two bytes indicate whether a record is a bh record, a bio
   record, a syscall record or application-defined record, and
   lower two bytes specify the concrete types */

/* log record types */
#define EKM_CAT_SHIFT (16)
#define EKM_CAT_MASK       ((-1)<<EKM_CAT_SHIFT)
#define EKM_CAT(t)         (((t) & EKM_CAT_MASK)>>EKM_CAT_SHIFT)

#define EKM_MINOR(t)       ((t)&((1<<EKM_CAT_SHIFT)-1))
#define EKM_MIN_MINOR (1)
#define EKM_MAX_MINOR ((1<<EKM_CAT_SHIFT)-1)
#define EKM_MAKE_TYPE(cat, minor) \
			   (((cat)<<EKM_CAT_SHIFT) | minor)

#define EKM_INVALID_CAT     (0)

/* one sector */
#define EKM_SECTOR_CAT       (1)
#define EKM_ADD_SECTOR       (1|(EKM_SECTOR_CAT<<EKM_CAT_SHIFT))
#define EKM_REMOVE_SECTOR    (2|(EKM_SECTOR_CAT<<EKM_CAT_SHIFT))
#define EKM_WRITE_SECTOR     (3|(EKM_SECTOR_CAT<<EKM_CAT_SHIFT))
#define EKM_READ_SECTOR      (4|(EKM_SECTOR_CAT<<EKM_CAT_SHIFT))

/* one more more blocks */
#define EKM_BLOCK_CAT       (2)
#define EKM_ADD_BLOCK       (1|(EKM_BLOCK_CAT<<EKM_CAT_SHIFT))
#define EKM_REMOVE_BLOCK    (2|(EKM_BLOCK_CAT<<EKM_CAT_SHIFT))
#define EKM_WRITE_BLOCK     (3|(EKM_BLOCK_CAT<<EKM_CAT_SHIFT))
#define EKM_READ_BLOCK      (4|(EKM_BLOCK_CAT<<EKM_CAT_SHIFT))

/* a generic buffer op */
#define EKM_BUFFER_CAT       (3)
#define EKM_ADD_BUFFER       (1|(EKM_BUFFER_CAT<<EKM_CAT_SHIFT))
#define EKM_REMOVE_BUFFER    (2|(EKM_BUFFER_CAT<<EKM_CAT_SHIFT))
#define EKM_WRITE_BUFFER     (3|(EKM_BUFFER_CAT<<EKM_CAT_SHIFT))
#define EKM_READ_BUFFER      (4|(EKM_BUFFER_CAT<<EKM_CAT_SHIFT))

#define EKM_SYSCALL_CAT     (4)

/* BEGIN SYSCALL */
/* 
 *   Everything between BEGIN SYSCALL and END SYSCALL 
 *   is automatically generated from ./scripts/gen_syscall_log.pl 
 */

#define EKM_enter_chdir (0|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_chdir (1|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_close (2|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_close (3|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_fdatasync (4|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_fdatasync (5|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_fsync (6|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_fsync (7|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_ftruncate (8|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_ftruncate (9|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_link (10|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_link (11|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_llseek (12|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_llseek (13|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_lseek (14|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_lseek (15|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_mkdir (16|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_mkdir (17|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_open (18|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_open (19|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_pwrite64 (20|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_pwrite64 (21|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_read (22|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_read (23|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_rename (24|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_rename (25|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_rmdir (26|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_rmdir (27|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_symlink (28|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_symlink (29|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_sync (30|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_sync (31|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_truncate (32|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_truncate (33|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_unlink (34|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_unlink (35|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_enter_write (36|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
#define EKM_exit_write (37|(EKM_SYSCALL_CAT<<EKM_CAT_SHIFT))
/* END SYSCALL */

#define EKM_RESERVED_CAT_BEGIN (16)
#define EKM_RESERVED_CAT_END (31)

#define EKM_USR_CAT_BEGIN   (32)
#define EKM_USR_CAT_END     (0xEF)


// embeded in every log record.  used to check long integrity
#define EKM_LOG_MAGIC (0xF12C0109)

// marks commit blocks
#define EKM_FL_COMMIT (1<<0)


typedef int rec_type_t; // record type
typedef int rec_cat_t; // record category

#endif /* ifdef __EKM_LOG_TYPES_H */
