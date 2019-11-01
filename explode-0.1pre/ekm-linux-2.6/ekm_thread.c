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


#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/jbd.h>
#include <linux/ext3_fs.h>
#include <linux/raid/md.h>

#include "ekm.h"
#include "ekm_thread.h"

#ifdef ENABLE_EKM_RUN
int ekm_run_thread(int pid) {
  long oldNice;
  struct task_struct *p;
  int error = -ESRCH;
  
  EKM_PRINTK("EKM: run thread %d", pid);
  read_lock(&tasklist_lock);
  p = find_task_by_pid(pid);
  if (p) {
    /* this gets around an EXPORT_SYMBOL and I don't think it will change */
    oldNice = p->prio - MAX_RT_PRIO;
    set_user_nice(p, -20);
    error = 0;
  } else {
    read_unlock(&tasklist_lock);
    return error;
  }
  read_unlock(&tasklist_lock);
  if(error)
    return error;

  wake_up_process(p);
  yield();
  
  read_lock(&tasklist_lock);
  p = find_task_by_pid(pid);
  if (p) {
    set_user_nice(p, oldNice);
  }
  read_unlock(&tasklist_lock);
  
  return 0;
}

int ekm_ran_thread(int pid)
{
  struct task_struct *p;
  int last_ran = 0;

  read_lock(&tasklist_lock);
  p = find_task_by_pid(pid);
  if(p)
	  last_ran = 0;//p->timestamp;
  read_unlock(&tasklist_lock);

  return last_ran;
}
#endif

int ekm_norun_thread(int pid)
{
  struct task_struct *p;
  EKM_PRINTK("SBD: setting low priority for %d\n", pid);
  read_lock(&tasklist_lock);
  p = find_task_by_pid(pid);
  if (p) {
    //extern int fail_pid;
    set_user_nice(p, 19);
    //fail_pid = pid;
  }
  else {
    read_unlock(&tasklist_lock);
    return -ESRCH;
  }
  read_unlock(&tasklist_lock);
  return 0;
}

#ifdef ENABLE_EKM_GET_PID
int ekm_get_pid(struct ekm_get_pid_req *req) {
  int error = -EINVAL;
  struct block_device *bdev;
  struct super_block *sb = NULL;
  bdev = lookup_bdev(req->path);
  if(IS_ERR(bdev))
    return PTR_ERR(bdev);
  
  error = -ENODEV;
  sb = get_super(bdev);
  if(!sb) 
    goto drop_bdev;
  
  /* YJF: this is really ad hoc */
  /*if(ekm_debug)
    printk ("EKM_GET_PID %s\n", 
    sb->s_type->name);*/
  
  if(!strcmp(sb->s_type->name, "ext3")) {
    journal_t *journal;
    pid_t pid;
    
    if(!(journal = EXT3_SB(sb)->s_journal))
      goto drop_sb;
    if(!(journal->j_task))
      goto drop_sb;
    
    pid = journal->j_task->pid;
    /*if(ekm_debug >= 2)
      printk ("EKM_GET_PID %d\n", pid);*/
    req->pid = pid;
  } else {
    EKM_PRINTK (KERN_ERR "EKM_GET_PID called on"\
	    " unsurpported file system %s", 
	    sb->s_type->name);
  }
 drop_sb:
  drop_super(sb);
 drop_bdev:
  bdput(bdev);
  return error;
}
#endif
