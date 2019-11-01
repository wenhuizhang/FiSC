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


#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bio.h>
#include <sys/explode.h>
#include <sys/conf.h>
#include <fs/devfs/devfs_int.h>
#include <geom/geom.h>

#include "ekm_log.h"

#define MAX_LOG_DEVICES 128

#define EKM_LOG_DATA_SIZE \
	(PAGE_SIZE - sizeof(struct ekm_log *) - sizeof(long))
typedef struct ekm_log {
  struct ekm_log *next;
  long size;
  char data[EKM_LOG_DATA_SIZE];
} ekm_log;

/* this is to synchronize access to the main log */
//spinlock_t log_lock = SPIN_LOCK_UNLOCKED;
struct mtx log_mutex;
//MTX_SYSINIT(ekm_log, &log_mutex, "Protects the ekm log", MTX_DEF);

ekm_log *ekm_log_start = 0;
ekm_log *ekm_log_cur = 0;
long ekm_log_size = 0;
int ekm_lsn = 0;

int ekm_num_pages = 0;


#define INLINE
//#define INLINE inline
static void empty_log_helper(ekm_log *log);
void log_add_buf(const char *data, long data_bytes);

struct log_device {
  unsigned int dev;
  struct g_geom *gp;
  int on;
  //enum fs_e fs;
};



struct log_device log_devices[MAX_LOG_DEVICES];

static int num_log_devices = 0;

MALLOC_DECLARE(M_EKMLOGPAGE);
MALLOC_DEFINE(M_EKMLOGPAGE, "ekmlogpage", "a page of the buffer log for ekm");


static int add_log_page(void) {
  ekm_log *next;
  //disable_choose();
  // for now let's not sleep since we aren't sure if we can until we figure out the locking rules better
  next = (ekm_log *) malloc(PAGE_SIZE, M_EKMLOGPAGE, M_NOWAIT);
  //enable_choose();
  if (next == 0) {
    printf("EKM error: can't get memory for log\n");
    // YJF: just die
    //BUG();
    return 0;
  }

#ifdef EKM_DEBUG
  if(ekm_num_pages > 1000)
	  printf("EKM: %d > 1000 pages\n", ekm_num_pages);
#endif

  //printf("EKM: add log page %p, total = %d\n", next, ekm_num_pages);
  ekm_num_pages ++;
  if (ekm_log_start == 0)
    ekm_log_start = next;
  else
    ekm_log_cur->next = next;
  ekm_log_cur = next;
  ekm_log_cur->next = 0;
  ekm_log_cur->size = 0;
  return 1;
}

void log_add_buf(const char *data, long data_bytes) {
  /* Until we figure out why we're getting bad pointers passed
     in, we might as well avoid dereferencing them. */
  /* The above is the funniest comment I've read in a while :-) - Can */
  /*if (INVALID(data)) {
    printf("EKM error: can't log data from invalid pointer %p\n", data);
    return;
    }*/

  while (data_bytes > 0) {
    int bytes_available;
    int copy_bytes;

    if (ekm_log_cur == 0 || ekm_log_cur->size >= EKM_LOG_DATA_SIZE) {
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
}

// make them explicit, so we know exactly how to unmarshal
static void log_add_int(int num)
{
	log_add_buf((char*)&num, sizeof num);
}

static void log_add_long(long num)
{
	log_add_buf((char*)&num, sizeof num);
}

static int get_buf_dev(struct buf *buf)
{
  struct vnode *vp = buf->b_vp;
  struct vattr vap;
  struct ucred cred;

  //printf("Buf %p Vp %p\n", buf, vp);

  if(!vp)
    return 0;

  if(VOP_GETATTR(vp, &vap, &cred, NULL)) {
    printf("error\n");
    return 0;
  }

  /*if(vap.va_type !=  VREG)
    printf("Type %d\n", vap.va_type);

  if(vap.va_type != VREG)
    printf("Rdev %d\n", vap.va_rdev);

  if(vap.va_type != VREG)
  printf("Dev %d\n", vap.va_fsid);*/

  return vap.va_rdev;
  //return 0;
}

void log_add_bh_entry(struct buf *bh, int type) {
        int size = sizeof (struct ekm_bh_record);
	int flags = 0;
	if(EKM_REC_TYPE_DATA_P(type))
		size += bh->b_bcount;
	log_add_int(size);
	log_add_int(ekm_lsn++);
	log_add_int(type);
	log_add_int(curthread->td_proc->p_pid);
	log_add_int(EKM_LOG_MAGIC);
	log_add_int(get_buf_dev(bh));
	/*if(ekm_is_commit(bh->b_bdev, (void*)bh, 0))
	  flags |= EKM_FL_COMMIT;*/
	log_add_int(flags);
	printf("Loc %d Count %ld\n", (int)bh->b_lblkno, buf->b_bcount);
	log_add_long(bh->b_lblkno); //should this be physical (think so) or logical?
	log_add_int(bh->b_bcount);
	if(EKM_REC_TYPE_DATA_P(type))
	  log_add_buf(bh->b_data, bh->b_bcount);
}

static int find_dev(struct bio *bio)
{
	int i;
	struct g_geom *gp;
	if(!bio || !bio->bio_to || !(gp=bio->bio_to->geom))
		return -1;
	for(i=0;i<num_log_devices;i++)
		if(log_devices[i].on && gp == log_devices[i].gp)
			return log_devices[i].dev;
	return -1;
}

void log_add_req_entry(struct bio *bio, int type) {
  //struct bio *bio;
	int size = sizeof (struct ekm_bio_record);
	int flags = 0;
	//bio = req->bio;
	// can be converted
	if(EKM_REC_TYPE_DATA_P(type))
	  size += bio->bio_length;
	log_add_int(size);
	// no data
	log_add_int(ekm_lsn++);
	log_add_int(type);
	log_add_int(curthread->td_proc->p_pid);
	log_add_int(EKM_LOG_MAGIC);
	// change
	//printf("bio_disk %p\n",bio->bio_disk);
	printf("Dev %d\n", find_dev(bio));
	log_add_int(find_dev(bio));
	// commit flag doesn't matter here
	//if(ekm_is_commit(bio->bi_bdev, (void*)bio, 1))
	//	flags |= EKM_FL_COMMIT;
	log_add_int(flags);
	log_add_long((long) bio->bio_offset/512);
	//printf("BioCount %ld\n", (long) bio->bio_length);
	// convert # of sectors to size
	log_add_int(bio->bio_length);
	if(EKM_REC_TYPE_DATA_P(type))
		log_add_buf(bio->bio_data, bio->bio_length);	
}

long INLINE get_log_size(void)
{
  long temp;
  //unsigned long flags;
  mtx_lock(&log_mutex);
  //spin_lock_irqsave(&log_lock, flags);
  temp = ekm_log_size;
  mtx_unlock(&log_mutex);
  //spin_unlock_irqrestore(&log_lock, flags);
  return temp;
}

// probably rewrite this because copyout mightw work with locks held
int copy_log_buffer(char * buffer, long *size) {
  ekm_log *temp_start;
  ekm_log *temp_cur;
  long copied = 0;
  //unsigned long flags;

  mtx_lock(&log_mutex);
  //spin_lock_irqsave(&log_lock, flags);
  temp_start = ekm_log_start;

  if(ekm_log_size > *size) {
    mtx_unlock(&log_mutex);
    //printf("Overflow!\n");
    //spin_unlock_irqrestore(&log_lock, flags);
    return -EOVERFLOW;
  }
  temp_cur = temp_start = ekm_log_start;
  ekm_log_start = ekm_log_cur = NULL;
  ekm_log_size = 0;
  mtx_unlock(&log_mutex);
  //spin_unlock_irqrestore(&log_lock, flags);

  while(temp_cur != NULL) {
    if(copyout(temp_cur->data, buffer +  copied, temp_cur->size)) {
      //printf("Copyout failed!\n");
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
  // unsigned long flags;
  mtx_lock(&log_mutex);
  //spin_lock_irqsave(&log_lock, flags);

  empty_log_helper(ekm_log_start);

  ekm_log_start = 0;
  ekm_log_cur = 0;
  ekm_log_size = 0;
  ekm_lsn = 0;

  mtx_unlock(&log_mutex);
  //spin_unlock_irqrestore(&log_lock, flags);
}

/* this must be called with the lock held */
static void empty_log_helper(ekm_log *log) {
  ekm_log *temp;

  while(log != 0) {
    temp = log->next;
    free(log, M_EKMLOGPAGE); // CHANGE
    ekm_num_pages --;
    //printf("EKM: log free page %p, total = %d\n", log, ekm_num_pages);
    log = temp;
  }
}

/* get the array index for a device, called with lock held  */
static INLINE int find_device_index(int dev, int start) {
  int i;

  /*printk("Finding %d\n", new_encode_dev(dev->bd_dev));*/

  for(i = start; i < num_log_devices; i++) {
    /* change this check (minor) later to allow for npartitions != 1 */
    if(log_devices[i].dev == dev)
      return i;
  }
  return -1;
}

/* YJF: this is broken.  one dev can have multiple aliases. */
int is_traced_dev(struct buf *bh) {
  return is_traced_udev(get_buf_dev(bh));
}

int  is_traced_udev(int dev) {
  int index;
  //unsigned long flags;

  int on = 0;
  int start = 0;

  mtx_lock(&log_mutex);

  /*if(spin_is_locked(&log_lock)) {
	  dump_stack();
  }
  
  spin_lock_irqsave(&log_lock, flags);*/
  while(start < num_log_devices) {
	  index = find_device_index(dev, start);
	  if(index >= 0) {
		  /* look for next device */
		  on |= log_devices[index].on;
		  start = index + 1;
		  /*printk("Start %d\n", start);*/
	  } else /* no more */
		  break;
  }
  mtx_unlock(&log_mutex);
  //spin_unlock_irqrestore(&log_lock, flags);

  // EXPLODE
  return on;
  //return 0;
}

int is_traced_geom(struct g_geom *gp)
{
	int ret = 0, i;
	mtx_lock(&log_mutex);
	for(i=0;i<num_log_devices;i++)
		if(log_devices[i].on && log_devices[i].gp == gp)
			ret = 1;
	mtx_unlock(&log_mutex);

	if(ret)
		printf("is_traced_geom returns true\n");

	return ret;
}

static INLINE int new_dev_index(int dev) {
  int i;
  for(i = 0; i < num_log_devices; i++) {
    if(log_devices[i].dev == dev || !log_devices[i].on)
      log_devices[i].dev = dev;
      return i;
  }

  if(num_log_devices >= MAX_LOG_DEVICES)
    return -1;

  num_log_devices++;
  log_devices[num_log_devices-1].dev = dev;

  return num_log_devices-1;
}

/* get the array index for a particular handle, called with lock held  */
static INLINE int find_dev_index(int dev) {
  int i;
  for(i = 0; i < num_log_devices; i++) {
    if(log_devices[i].dev == dev)
      return i;
  }
  return -1;
}

extern TAILQ_HEAD(,cdev_priv) cdevp_list;
int start_trace(int dev) {
  int index;
  //unsigned long flags;
  struct cdev_priv *cdp;
  struct g_consumer *cp;

  mtx_lock(&log_mutex);
  //spin_lock_irqsave(&log_lock, flags);
  index = new_dev_index(dev);

  if(index < 0) {
    mtx_unlock(&log_mutex);
    //spin_unlock_irqrestore(&log_lock, flags);
    return -1;
  }
  log_devices[index].on = 1;

  mtx_unlock(&log_mutex);

  dev_lock();
  TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
	  if(dev == dev2udev(&cdp->cdp_c)) {
		  // refer to geom_dev.c
		  cp = cdp->cdp_c.si_drv2;
		  printf("cp = %p\n", cp);
		  printf("cp->provider = %p\n", cp->provider);
		  printf("cp->provider->geom = %p\n", cp->provider->geom);
		  log_devices[index].gp = cp->provider->geom;
	  }
  }
  dev_unlock();


  for(index=0;index<num_log_devices;index++)
	  printf("all traced dev %u\n", log_devices[index].dev);
  //spin_unlock_irqrestore(&log_lock, flags);
  return 0;
}

int stop_trace(int dev) {
  int index;
  //unsigned long flags;

  mtx_lock(&log_mutex);
  //spin_lock_irqsave(&log_lock, flags);
  index = find_dev_index(dev);
  if(index < 0) {
    mtx_unlock(&log_mutex);
    //spin_unlock_irqrestore(&log_lock, flags);
    return -1;
  }
  log_devices[index].on = 0;
  mtx_unlock(&log_mutex);
  //spin_unlock_irqrestore(&log_lock, flags);
  return 0;
}

// not needed any longer?
//int ekm_set_fs(int handle, enum fs_e fs)

void stop_traces(void) {
  int i;
  //unsigned long flags;
  //spin_lock_irqsave(&log_lock, flags);
  for(i = 0; i < num_log_devices; i++)
	  log_devices[i].on = 0;
  //spin_unlock_irqrestore(&log_lock, flags);
}

#if 0
/* helper function, called with lock held */
static int traces_on(void) {
  int i;
  for(i = 0; i < num_log_devices; i++)
    if(log_devices[i].on == 1)
      return 1;

  return 0;
}
#endif

void ekm_log_app_record(struct ekm_generic_record *gr)
{
  //unsigned long flags;	
  //spin_lock_irqsave(&log_lock, flags);
  mtx_lock(&log_mutex);
  gr->hdr.lsn = ekm_lsn++;
  gr->hdr.magic = EKM_LOG_MAGIC;
  log_add_buf((char*)gr, gr->hdr.size);
  //spin_unlock_irqrestore(&log_lock, flags);
  mtx_unlock(&log_mutex);
}
