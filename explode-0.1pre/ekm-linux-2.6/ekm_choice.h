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


#ifndef __EKM_CHOICE_H
#define __EKM_CHOICE_H

#ifndef __KERNEL__
#	include <sys/types.h>

/* this enum must be identical to the same enum defined in
   $(KERNEL_DIR)/linux-2.6.11/include/explode.h */
/* Choice point identifiers. */
enum choice_point {
	EKM_KMALLOC = 1,
	EKM_USERCOPY = 2,
	EKM_BREAD = 4,
	EKM_SCHEDULE = 8,
	EKM_CACHE_READ = 16,
	EKM_DISK_READ = 32,
	EKM_DISK_WRITE = 64,
};

#endif 

struct ekm_choice {
  /* Number of choices to pass before failing, -1 to disable. */
  int pass_count;

  /* Value to return from count'th choice, typically 1.  0 is "success". */
  int possibility;

  /* Number of consecutive additional failures on the same choice
     point after the first.  Normally 0 but if you want to, say,
     fail kmalloc() many times in a row you can set it to a large
     number.  Use -1 to always fail after the first time, but
     that may not be a good idea because sometimes kernel code
     will loop forever waiting for an operation to succeed. */
  int extra_failures;

  /* Choice point filter.
     If set to 0, we will fail any choice point when pass_count
     reaches 0.
     If set nonzero, we will fail only the given choice point
     when pass_count reaches 0.  We still decrement pass_count
     regardless of the choice point.  If pass_count reaches 0 but
     the choice point is wrong, we fail the first subsequent
     correct choice point instead. */
  int choice;
  /* Pid filter.
     Only choice points reached in the given process are failed.
     A value of 0 is interpreted as the process that made the
     ioctl call. */
  pid_t pid[16];

  /* Number of times the pid filter matched, that is,
     the total number of choice points encountered for the given
     pid. */
  unsigned pid_matches;
};

#ifdef __KERNEL__
void ekm_choice_install(void);
void ekm_choice_uninstall(void);

void ekm_set_choice(const struct ekm_choice *);
void ekm_get_choice(struct ekm_choice *);

void ekm_disable_choices(void);
void ekm_enable_choices(void);

void ekm_kill_disk(u32 dev);
void ekm_revive_disk(u32 dev);
#endif

#endif /* ekm_choice.h */
