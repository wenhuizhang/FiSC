#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bio.h>
#include <sys/explode.h>
#include <sys/conf.h>
#include <fs/devfs/devfs_int.h>
#include <geom/geom.h>
#include <sys/kdb.h>

#include "ekm_log.h"
#include "ekm_log_generated.h"

extern struct mtx log_mutex;

static void ekm_bh_dirty(struct buf *bh)
{
  //unsigned long flags;
    if(!bh || !is_traced_dev(bh) || !bh->b_bcount)
        return;
    printf("Dirtying Buffer: ");
    /*if(INVALID(bh->b_data)) {
        printk(KERN_ERR "invalid block %d bh->b_data %p size %u\n", (int)bh->b_b
locknr, bh->b_data, bh->b_size);
        breakpoint();
        return;
    }
    spin_lock_irqsave(&log_lock, flags);*/

    mtx_lock(&log_mutex);
    log_add_bh_entry(bh, EKM_BH_write);
    mtx_unlock(&log_mutex);
    /*log_add_bh_entry(bh, EKM_BH_dirty);
    spin_unlock_irqrestore(&log_lock, flags);
    ekm_bh_tracer(EKM_BH_dirty, bh);*/
}


#if 1
static void ekm_bh_wait(struct buf *bh)
{
  //unsigned long flags;
  //printf("CALLED!\n");
  if(!bh || !is_traced_dev(bh) || !bh->b_bcount)
        return;

    printf("Waiting Buffer!\n");
    /*if(INVALID(bh->b_data)) {
        printk(KERN_ERR "invalid block %d bh->b_data %p size %u\n", (int)bh->b_b
locknr, bh->b_data, bh->b_size);
        breakpoint();
        return;
	}*/
    //spin_lock_irqsave(&log_lock, flags);
    mtx_lock(&log_mutex);
    log_add_bh_entry(bh, EKM_BH_wait);
    mtx_unlock(&log_mutex);
    //spin_unlock_irqrestore(&log_lock, flags);
    //ekm_bh_tracer(EKM_BH_wait, bh);
}
#endif

static void ekm_bh_write(struct buf *bh)
{
  //unsigned long flags;
    if(!bh || !is_traced_dev(bh) || !bh->b_bcount)
        return;
    printf("Writing Buffer: ");
    //kdb_backtrace();
    /*if(INVALID(bh->b_data)) {
        printk(KERN_ERR "invalid block %d bh->b_data %p size %u\n", (int)bh->b_b
locknr, bh->b_data, bh->b_size);
        breakpoint();
        return;
	}*/
    //spin_lock_irqsave(&log_lock, flags);
    mtx_lock(&log_mutex);
    log_add_bh_entry(bh, EKM_BH_write);
    mtx_unlock(&log_mutex);
    //spin_unlock_irqrestore(&log_lock, flags);
    //ekm_bh_tracer(EKM_BH_wait, bh);
}

static void ekm_bh_done(struct buf *bh)
{

  //unsigned long flags;
  if(!bh || !is_traced_dev(bh) || !bh->b_bcount) {
        return;
  }
  printf("Buffer Done: ");
    /*if(INVALID(bh->b_data)) {
        printk(KERN_ERR "invalid block %d bh->b_data %p size %u\n", (int)bh->b_b
locknr, bh->b_data, bh->b_size);
        breakpoint();
        return;
	}*/
    //spin_lock_irqsave(&log_lock, flags);
    mtx_lock(&log_mutex);
    log_add_bh_entry(bh, EKM_BH_done);
    mtx_unlock(&log_mutex);
    //spin_unlock_irqrestore(&log_lock, flags);
    //ekm_bh_tracer(EKM_BH_wait, bh);
}

#if 1
static void ekm_bio_start_write(struct bio *bio)
{
  //unsigned long flags;
  //struct bio_vec *bvec;
  //int i;

  if(!bio||!(bio->bio_cmd & BIO_WRITE) ||!bio->bio_to
     ||!bio->bio_to->geom || !is_traced_geom(bio->bio_to->geom))
    return;

    printf("Start Write!\n");
    /*__bio_for_each_segment(bvec, bio, i, 0) {
        char *addr = page_address(bvec->bv_page) + bvec->bv_offset;
        // YJF: this happens.  skip the entire record in this case 
        if(INVALID(addr)) {
            printk(KERN_ERR "invalid bio %d\n", (int)bio->bi_sector);
            breakpoint();
            return;
        }
    }*/

    mtx_lock(&log_mutex);
    log_add_req_entry(bio, EKM_BIO_start_write);
    mtx_unlock(&log_mutex);
/*spin_lock_irqsave(&log_lock, flags);
    log_add_bio_entry(bio, EKM_BIO_start_write);
    spin_unlock_irqrestore(&log_lock, flags);
    ekm_bio_tracer(EKM_BIO_start_write, bio);*/
}

static void ekm_req_write_done(struct bio *bio)
{
  /*unsigned long flags;
    struct bio *bio;
    if(!req || !req->bio)
        return;
	bio = req->bio;*/
  if(!bio||!(bio->bio_cmd & BIO_WRITE) ||!bio->bio_to
     ||!bio->bio_to->geom || !is_traced_geom(bio->bio_to->geom))
    return;

  //printf("Bio Write!\n");
  //spin_lock_irqsave(&log_lock, flags);
    mtx_lock(&log_mutex);
    log_add_req_entry(bio, EKM_BIO_write_done);
    mtx_unlock(&log_mutex);
    /*ekm_bio_tracer(EKM_BIO_write, bio);
    spin_unlock_irqrestore(&log_lock, flags);*/
}

static void ekm_req_read(struct bio *bio)
{
  /*unsigned long flags;
    struct bio *bio;
    if(!req || !req->bio)
        return;
	bio = req->bio;*/
  if(!bio||!(bio->bio_cmd & BIO_READ) ||!bio->bio_to
     ||!bio->bio_to->geom || !is_traced_geom(bio->bio_to->geom))
    return;
  //printf("Bio Read!\n");
    //spin_lock_irqsave(&log_lock, flags);
    mtx_lock(&log_mutex);
    log_add_req_entry(bio, EKM_BIO_read);
    mtx_unlock(&log_mutex);
    //ekm_bio_tracer(EKM_BIO_read, bio);
    //spin_unlock_irqrestore(&log_lock, flags);*/
}
#endif

void ekm_log_install(void)
{
    ek_hooks.bh_dirty = ekm_bh_dirty;
    ek_hooks.bh_wait = ekm_bh_wait;
    ek_hooks.bh_write = ekm_bh_write;
    ek_hooks.bh_done = ekm_bh_done;
    ek_hooks.bio_start_write = ekm_bio_start_write;
    ek_hooks.req_write_done = ekm_req_write_done;
    ek_hooks.req_read = ekm_req_read;
}

void ekm_log_uninstall(void)
{
    ek_hooks.bh_dirty = NULL;
    ek_hooks.bh_wait = NULL;
    ek_hooks.bh_write = NULL;
    ek_hooks.bh_done = NULL;
    ek_hooks.bio_start_write = NULL;
    ek_hooks.req_write_done = NULL;
    ek_hooks.req_read = NULL;
}
