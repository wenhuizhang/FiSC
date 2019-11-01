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


#ifndef __EKM_THREAD_H
#define __EKM_THREAD_H

//#ifdef __KERNEL__

#ifdef ENABLE_EKM_RUN
int ekm_run_thread(int pid);
int ekm_ran_thread(int pid);
#endif


int ekm_ran_thread(int pid);

//int ekm_norun_thread(int pid);

#ifdef ENABLE_EKM_GET_PID
int ekm_get_pid(struct ekm_get_pid_req *req);
#endif

//#endif /* __KERNEL__ */

#endif
