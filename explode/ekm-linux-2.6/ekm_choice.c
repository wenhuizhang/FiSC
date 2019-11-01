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


#include <linux/hardirq.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/explode.h>
#include <linux/blkdev.h>

#include "ekm.h"
#include "ekm_choice.h"

/* need a clean implementation */

static struct ekm_choice fail;

/* for failing disk requests */
#define MAX_FAILED_DISKS (10)
static dev_t failed_disks[MAX_FAILED_DISKS];
static int n_failed_disks = 0;

/* used to filter out irrelavent choose calls */
typedef int (*filter_fn_t)(const struct ekm_choice*, void*);

void
ekm_set_choice(const struct ekm_choice *sc) 
{
  fail = *sc;
#if 0
  if (fail.pid == 0)
	  fail.pid = current->pid;
#endif
  fail.pid_matches = 0;
#if 0
 {
	int size, i;
	size = sizeof(sc->pid)/sizeof(sc->pid[0]);
	for(i = 0; i < size && sc->pid[i]; i++)
		printk("EKM: pid = %d\n", sc->pid[i]);
 }
#endif

}

void
ekm_get_choice(struct ekm_choice *sc) 
{
  *sc = fail;
  /* YJF: always reset after a call to get_choice */
  fail.pass_count = -1;
}

/* Returns the path to take, out of N_POSSIBILITIES
   possibilities, at choice point CP.  The default choice point
   is always 0.

   This is installed as the `ekm_choose' handler. */
static int
//ekm_mod_choose(enum choice_point cp, int n_possibilities) 
ekm_mod_filter_choose(enum choice_point cp, int n_possibilities, 
		      filter_fn_t filter, void* arg)
{
  /* Don't fail if we're processing an interrupt or if we're not
     in the right process.  This may give us some semblance of
     reproducibility. */
#if 0
	if(!in_interrupt() && fail.pass_count >= 0 
	   && choose_enabled()
	   && filter(&fail, arg)) {
		/* && pid_match(&fail, current->pid)) { */
		printk("EKM: in choose, pid = %d\n",
		       current->pid);
		dump_stack();
	}
  if(cp == EKM_DISK_READ || cp == EKM_DISK_WRITE) {
	  printk("EKM: in choose. pass count = %d, fail.choice = %x, cp = %x\n", 
		 fail.pass_count, fail.choice, cp);
	  printk("EKM: in_intrrupt %d, filter %d, enabled %d\n", 
		 (int)in_interrupt(),
		 (int)filter(&fail, arg),
		 (int)choose_enabled());
	  dump_stack();
  }
	if(cp & (cp -1))
		BUG();
#endif


  if (in_interrupt() || !filter(&fail, arg)
      || !choose_enabled())
    return 0;

  fail.pid_matches++;

  /* Don't fail if we're disabled. */
  if (fail.pass_count < 0)
    return 0;

  /* Wrong choice point? */
  if (fail.choice != 0 && !(fail.choice&cp))
    return 0;

  /* YJF: do this after we checked choice */
  /* Any passes left? */
  if (fail.pass_count > 0) {
    fail.pass_count--;
    return 0;
  }

  /* printk("EKM: pass count is 0. should fail\n"); */

  /* Now we know that we will fail this call with return value
     fail.possibility.  Update pass_count, extra_failures, and
     choice so that the next call to sbd_choose() will do the
     right thing. */
  if (fail.extra_failures == 0) {
    /* No failures left.  Disable. */
    fail.pass_count = -1;
  } else {
    /* Fail only the same choice point later. */
    fail.choice = cp;
    fail.extra_failures--;
  }

  printk("EKM: failing choice point %d\n", cp);
  dump_stack();
  if (fail.possibility >= n_possibilities)
    fail.possibility = 1;
  return fail.possibility;
}

static int
pid_match(const struct ekm_choice *sc, void *arg)
{
	int size, i;
	pid_t pid = (pid_t)arg;
	size = sizeof(sc->pid)/sizeof(sc->pid[0]);
	for(i = 0; i < size && sc->pid[i]; i++)
		if(sc->pid[i] == pid)
			return 1;
	return 0;
}

static int
ekm_mod_choose(enum choice_point cp, int n_possibilities) 
{
	return ekm_mod_filter_choose(cp, n_possibilities, 
				     pid_match, (void*)(current->pid));
}

void 
ekm_kill_disk(u32 d)
{
	if(n_failed_disks >= MAX_FAILED_DISKS) {
		printk("EKM: can't kill disk %x\n", d);
			return;
	}
			
	failed_disks[n_failed_disks++] = new_decode_dev(d);
	printk("EKM: killed disk %x\n", d);
}

void 
ekm_revive_disk(u32 d)
{
	dev_t dev;
	int i;

	dev = new_decode_dev(d);
	for(i = 0; i < n_failed_disks; i++)
		if(failed_disks[i] == dev)
			break;

	if(i == n_failed_disks) {
		printk("EKM: can't revive disk %x\n", d);
		return;
	}

	for(; i < n_failed_disks - 1; i++)
		failed_disks[i] = failed_disks[i+1];
	n_failed_disks --;

	printk("EKM: revived disk %x\n", d);
}

static int
req_dev_match(const struct ekm_choice *sc, void *arg)
{
	dev_t dev;
	int i;
	struct request *req = (struct request*)arg;

	if(!req || !req->bio || !req->bio->bi_bdev)
		return 0;
	dev = req->bio->bi_bdev->bd_dev;

	for(i = 0; i < n_failed_disks; i ++) {
		if(failed_disks[i] == dev) 
			return 1;
	}

	return 0;
}

static int 
ekm_mod_fail_req(struct request *req)
{
	if(req_dev_match(&fail, req)) {
		EKM_PRINTK("EKM: fail req called\n");
	}
	if(rq_data_dir(req))
		return ekm_mod_filter_choose(EKM_DISK_WRITE, 2, 
					     req_dev_match, (void*)req);
	else
		return ekm_mod_filter_choose(EKM_DISK_READ, 2, 
					     req_dev_match, (void*)req);
}

void /*__init*/
ekm_choice_install(void) 
{
  ek_hooks.choose = ekm_mod_choose;
  ek_hooks.fail_req = ekm_mod_fail_req;
}

void /*__exit */
ekm_choice_uninstall(void) 
{
  ek_hooks.choose = NULL;
  ek_hooks.fail_req = NULL;
}


