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


#include "ekm.h"

#define MAX_LOG_DEVICES 128

static int want_read = 0;

struct log_device {
        unsigned int dev; // 0: free slot
        int on:1; // 0: not traced,  1: traced
};

struct log_device log_devices[MAX_LOG_DEVICES];
static int n_log_dev = 0;

static inline int find_dev_index(int dev, int cr);
static int get_dev_handle(struct block_device *dev);

// return true if p is a commit block
int ekm_is_commit(struct block_device *dev, void *p, int is_bio)
{
        // TODO: This does nothing now.  
        return 0;
}

int start_trace(struct ekm_start_trace_request *req) {
        int index;
        unsigned long flags;

        spin_lock_irqsave(&log_lock, flags);
        index = find_dev_index(req->dev, 1);

        if(index < 0) {
                spin_unlock_irqrestore(&log_lock, flags);
                return -1;
        }

        if(log_devices[index].on) {
                printk(KERN_ERR "can't start trace on %d: already tracing it\n",
                       req->dev);
                spin_unlock_irqrestore(&log_lock, flags);
                return -1;
        }
                
        log_devices[index].on = 1;
        want_read = req->want_read;

        spin_unlock_irqrestore(&log_lock, flags);
        return 0;
}

int stop_trace(int dev) {
        int index;
        unsigned long flags;

        spin_lock_irqsave(&log_lock, flags);
        index = find_dev_index(dev, 0);
        if(index < 0) {
	        printk("can't find dev %d\n", dev);
                spin_unlock_irqrestore(&log_lock, flags);
                return -1;
        }

        if(log_devices[index].on == 0) {
                printk(KERN_ERR "can't stop trace on %d: not tracing it\n",
                       dev);
                spin_unlock_irqrestore(&log_lock, flags);
                return -1;
        }

        log_devices[index].on = 0;

        spin_unlock_irqrestore(&log_lock, flags);
        return 0;
}

void stop_traces() {
        int i;
        unsigned long flags;
        spin_lock_irqsave(&log_lock, flags);

        for(i = 0; i < n_log_dev; i++)
                if(log_devices[i].on)
                        log_devices[i].on = 0;

        spin_unlock_irqrestore(&log_lock, flags);
}

static int traces_on(void) {
        int i;

        for(i = 0; i < n_log_dev; i++)
                if(log_devices[i].on)
                        return 1;

        return 0;
}

int ekm_is_traced_dev(int dev)
{
        int i, on;
        unsigned long flags;

        if(spin_is_locked(&log_lock))
                dump_stack();

        spin_lock_irqsave(&log_lock, flags);
        i = find_dev_index(dev, 0);
        if(i < 0) {
                spin_unlock_irqrestore(&log_lock, flags);
                return 0;
        }
        on = log_devices[i].on;
        spin_unlock_irqrestore(&log_lock, flags);
        
        if(on) EKM_PRINTK("Device %d on\n", on);

        return on;
}

/* returns non-zero if we're tracing this bdev */
static int is_traced_bdev(struct block_device *bdev) {
        if(!bdev)
                return 0;
        return ekm_is_traced_dev(get_dev_handle(bdev));
}

/* get the array index for a particular handle, called with lock held  */
static inline int find_dev_index(int dev, int cr) {
        int i;

        for(i = 0; i < n_log_dev; i++)
                if(log_devices[i].dev == dev)
                        return i;
        if(!cr) return -1;
        
        if(n_log_dev >= MAX_LOG_DEVICES)
                panic("ekm: can't trace more than %d devices\n", n_log_dev);

        // create a slot for this device
        log_devices[n_log_dev].dev = dev;
        return n_log_dev++;
}

/* get the handle for a device, called with lock held  */
static int get_dev_handle(struct block_device *dev) {
        // return a dev handle that makes sense for sys_stat, so we can
        // compare it to the st_rdev field we got in user-space
        return new_encode_dev(dev->bd_dev);
}

void ekm_log_app_record(struct ekm_app_record *gr)
{
        unsigned long flags;
        
        spin_lock_irqsave(&log_lock, flags);
        ekm_lsn++;
        log_add_int(gr->type);
        log_add_int(ekm_lsn);
        log_add_int(current->pid);
        log_add_int(EKM_LOG_MAGIC);
        log_add_sized_buf(gr->data, gr->len);
        spin_unlock_irqrestore(&log_lock, flags);
}

static void log_add_bh_entry(struct buffer_head *bh, int type) {
	int flags = 0;

	log_add_int(type);
	log_add_int(ekm_lsn++);
	log_add_int(current->pid);
	log_add_int(EKM_LOG_MAGIC);
	log_add_int(get_dev_handle(bh->b_bdev));
	if(ekm_is_commit(bh->b_bdev, (void*)bh, 0))
		flags |= EKM_FL_COMMIT;
	log_add_int(flags);
        // convert block num to sector num
	log_add_long((long)(bh->b_blocknr * (bh->b_size>>9)));
	log_add_int(bh->b_size);
	if(EKM_REC_TYPE_HAS_DATA(type))
		log_add_buf(bh->b_data, bh->b_size);
}

static void log_add_bio_entry(struct bio *bio, int type) {
	int flags = 0;
	int i;

	log_add_int(type);
	log_add_int(ekm_lsn++);
	log_add_int(current->pid);
	log_add_int(EKM_LOG_MAGIC);
	log_add_int(get_dev_handle(bio->bi_bdev));
	if(ekm_is_commit(bio->bi_bdev, (void*)bio, 1))
		flags |= EKM_FL_COMMIT;
	log_add_int(flags);
	// YJF: why divided by bio_sectors??
	//log_add_int(bio->bi_sector / bio_sectors(bio));
	log_add_long((long) bio->bi_sector);
	log_add_int(bio->bi_size);
	if(EKM_REC_TYPE_HAS_DATA(type)) {
		struct bio_vec *bvec;
		unsigned long total = 0;
		__bio_for_each_segment(bvec, bio, i, 0) {
			char *addr = page_address(bvec->bv_page) 
				+ bvec->bv_offset;
			unsigned int len = bvec->bv_len;
			log_add_buf(addr, len);
			total += len;
		}
		BUG_ON(total != bio->bi_size);
	}
}

static void log_add_req_entry(struct request *req, int type) {
	struct bio *bio;
	int flags = 0;

	bio = req->bio;
	log_add_int(type);
	log_add_int(ekm_lsn++);
	log_add_int(current->pid);
	log_add_int(EKM_LOG_MAGIC);
	log_add_int(get_dev_handle(bio->bi_bdev));
	// commit flag doesn't matter here
	if(ekm_is_commit(bio->bi_bdev, (void*)bio, 1))
		flags |= EKM_FL_COMMIT;
	log_add_int(flags);
	log_add_long((long) req->sector);
	// convert # of sectors to size
	log_add_int(req->current_nr_sectors << 9);
	if(EKM_REC_TYPE_HAS_DATA(type))
		log_add_buf(req->buffer, req->current_nr_sectors << 9);	
}

static void log_add_buffer_entry(int type, int dev, long sector, 
                                 int len, const char* buf)
{
	log_add_int(type);
	log_add_int(ekm_lsn++);
	log_add_int(current->pid);
	log_add_int(EKM_LOG_MAGIC);
	log_add_int(dev);
	log_add_int(/* flags.  UNUSED */ 0); 
	log_add_long(sector);
	log_add_int(len);
        /* all EKM_BUFFER_* records have data attached */
        log_add_buf(buf, len);
}

static void ekm_bh_tracer(int type, struct buffer_head *bh)
{
#ifdef DEBUG
	printk("BH: %d, %llu, %u\n", type, 
	       (unsigned long long) bh->b_blocknr, bh->b_size);
	dump_stack();
#endif
}

static void ekm_bio_tracer(int type, struct bio *bio)
{
#ifdef DEBUG
	if(bio->bi_bdev->bd_part)
		printk("BIO: %llu\n", 
		       (unsigned long long)bio->bi_bdev->bd_part->start_sect);
	printk("RDDD:  %llu, %u\n", 
	       (unsigned long long)bio->bi_sector, bio->bi_size);
	dump_stack();
#endif
}

#define EKM_BH(state) \
static void ekm_bh_##state(struct buffer_head *bh) \
{ \
        unsigned long flags; \
        if(!bh || !is_traced_bdev(bh->b_bdev) || !bh->b_size) \
                return; \
        if(INVALID(bh->b_data)) { \
                printk(KERN_ERR "invalid block %d bh->b_data %p size %u, "\
                       "probably lives in highmem\n",  \
                       (int)bh->b_blocknr, bh->b_data, (int)bh->b_size); \
                breakpoint(); \
                return; \
        } \
        spin_lock_irqsave(&log_lock, flags); \
        log_add_bh_entry(bh, EKM_BH_##state); \
        spin_unlock_irqrestore(&log_lock, flags); \
        ekm_bh_tracer(EKM_BH_##state, bh); \
}

EKM_BH(dirty)
EKM_BH(clean)
EKM_BH(lock)
EKM_BH(wait)
EKM_BH(unlock)
EKM_BH(write)
EKM_BH(write_sync)

#define EKM_BIO(state) \
void ekm_bio_##state(struct bio *bio) \
{ \
        unsigned long flags; \
        struct bio_vec *bvec; \
        int i; \
        if(!bio || !is_traced_bdev(bio->bi_bdev) || !(bio->bi_rw & WRITE)) \
                return; \
        __bio_for_each_segment(bvec, bio, i, 0) { \
                char *addr = page_address(bvec->bv_page) + bvec->bv_offset; \
                /* YJF: this happens.  skip the entire record in this case */ \
                if(INVALID(addr)) { \
                        printk(KERN_ERR "invalid bio %d, "\
                               "probably lives in highmem\n", \
                               (int)bio->bi_sector); \
                        breakpoint(); \
                        return; \
                } \
        } \
        spin_lock_irqsave(&log_lock, flags); \
        log_add_bio_entry(bio, EKM_BIO_##state); \
        spin_unlock_irqrestore(&log_lock, flags); \
        ekm_bio_tracer(EKM_BIO_##state, bio); \
}

EKM_BIO(start_write)
EKM_BIO(wait)

#define EKM_REQ(state, dir) \
void ekm_req_##state(struct request *req) \
{ \
        unsigned long flags; \
        struct bio *bio; \
        if(!req || !req->bio) \
                return; \
        bio = req->bio; \
        if(!is_traced_bdev(bio->bi_bdev) || !(bio->bi_rw & dir)) \
                return; \
        if(!want_read && (bio->bi_rw & READ)) \
                return; \
        spin_lock_irqsave(&log_lock, flags); \
        log_add_req_entry(req, EKM_BIO_##state); \
        ekm_bio_tracer(EKM_BIO_##state, bio); \
        spin_unlock_irqrestore(&log_lock, flags); \
}

EKM_REQ(write, WRITE)
EKM_REQ(read, READ)
EKM_REQ(write_done, WRITE)

static void ekm_syscall_log_insall(void);
void ekm_log_install(void)
{
        ek_hooks.bh_dirty = ekm_bh_dirty;
        ek_hooks.bh_clean = ekm_bh_clean;
        ek_hooks.bh_lock = ekm_bh_lock;
        ek_hooks.bh_wait = ekm_bh_wait;
        ek_hooks.bh_unlock = ekm_bh_unlock;
        ek_hooks.bh_write = ekm_bh_write;
        ek_hooks.bh_write_sync = ekm_bh_write_sync;
        ek_hooks.bio_start_write = ekm_bio_start_write;
        ek_hooks.bio_wait = ekm_bio_wait;
        ek_hooks.req_write = ekm_req_write;
        ek_hooks.req_read = ekm_req_read;
        ek_hooks.req_write_done = ekm_req_write_done;
        ek_hooks.log_buffer_record = log_add_buffer_entry;
        ekm_syscall_log_insall();
}

static void ekm_syscall_log_uninstall(void);
void ekm_log_uninstall(void)
{
        ek_hooks.bh_dirty = NULL;
        ek_hooks.bh_clean = NULL;
        ek_hooks.bh_lock = NULL;
        ek_hooks.bh_wait = NULL;
        ek_hooks.bh_unlock = NULL;
        ek_hooks.bh_write = NULL;
        ek_hooks.bh_write_sync = NULL;
        ek_hooks.bio_start_write = NULL;
        ek_hooks.bio_wait = NULL;
        ek_hooks.req_write = NULL;
        ek_hooks.req_read = NULL;
        ek_hooks.req_write_done = NULL;
        ek_hooks.log_buffer_record = NULL;
        ekm_syscall_log_uninstall();
}

static int ekm_kmallocs = 0;
static void* ekm_kmalloc(size_t s, int f)
{
	void *p = kmalloc(s, f);
	if(p) {
		ekm_kmallocs ++;
		if(ekm_kmallocs > 10)
			EKM_PRINTK("EKM: kmalloc %p, total = %d\n", 
                                   p, ekm_kmallocs);
	} else {
		// just die
		// BUG();
		EKM_PRINTK("EKM: kmalloc failed size=%d, flag=%d\n", s, f);
		dump_stack();
	}
	return p;
}

static void ekm_kfree(const void* p)
{
	ekm_kmallocs --;
	if(ekm_kmallocs > 10)
		EKM_PRINTK("EKM: kfree %p, total = %d\n", p, ekm_kmallocs);	
	kfree(p);
}

static char* ekm_getname(const char __user *name)
{
	char* kern_name = getname(name);
	if(!IS_ERR(kern_name)) {
		ekm_kmallocs++;
		if(ekm_kmallocs > 10)
			EKM_PRINTK("EKM: kmalloc %p, total = %d\n",
                                   kern_name, ekm_kmallocs);
	} else {// just die
		//BUG();
		EKM_PRINTK("EKM: getname failed\n");
		dump_stack();
	}
	return kern_name;
}

static void ekm_putname(const char* name)
{
	ekm_kmallocs --;
	if(ekm_kmallocs > 10)
		EKM_PRINTK("EKM: kfree %p, total = %d\n", name, ekm_kmallocs);
	putname(name);
}

#define STRNCMP(s, eq, c) (strncmp(s, c, strlen(c)) eq 0)
static int file_traced(const char *name)
{
	char *p=NULL;
	if(!name)
		return 0;

	if(!traces_on())
		return 0;
	
	// FIXME: now just check file name or mounted device name.
	// need a better way

	if(*name=='/')
		return STRNCMP(name, ==, "/mnt/sbd");

	if(!current->fs || !current->fs->pwdmnt 
	   || !(p=(current->fs->pwdmnt->mnt_devname)))
		return 0;
#if 0
	printk("dev %s, name %s, pwd %s, root %s\n",
	       p, name, 
	       current->fs->pwd->d_name.name,
	       current->fs->root->d_name.name);
#endif
	
	return STRNCMP(p, ==, "/dev/rdd");
}

static int fd_traced(int fd)
{
	struct file *file;
	struct block_device *bdev;
	int ret = 0;

	file = fget(fd);
	if (!file)
		return 0;
	if(!file->f_mapping || !file->f_mapping->host
	   || !file->f_mapping->host->i_sb
	   || !(bdev=file->f_mapping->host->i_sb->s_bdev))
		goto out;

	ret = is_traced_bdev(bdev);
	if(ret)
		EKM_PRINTK("EKM: bdev(%d,%d) is traced\n",
                           MAJOR(bdev->bd_dev), MINOR(bdev->bd_dev));
 out:
	fput(file);
	return ret;
}

EXPORT_SYMBOL(ekm_is_traced_dev);

/* BEGIN SYSCALL*/
/* 
 *   Everything between BEGIN SYSCALL and END SYSCALL 
 *   is automatically generated from ./scripts/gen_syscall_log.pl 
 */

    
void ekm_enter_chdir(int *entry_lsn, const char __user * filename)
{
    unsigned long irqflags;

    char* filename_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         filename_tmp = ekm_getname(filename);
         if(IS_ERR(filename_tmp)) goto out;
    
    if(!file_traced(filename)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_chdir\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_chdir);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(filename_tmp);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(filename_tmp)) ekm_putname(filename_tmp);
;
    enable_choose();
}
    
void ekm_exit_chdir(int *entry_lsn,  long  ret, const char __user * filename)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_chdir\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_chdir);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_close(int *entry_lsn, unsigned int  fd)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_close\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_close);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_close(int *entry_lsn,  long  ret, unsigned int  fd)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_close\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_close);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_fdatasync(int *entry_lsn, unsigned int  fd)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_fdatasync\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_fdatasync);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_fdatasync(int *entry_lsn,  long  ret, unsigned int  fd)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_fdatasync\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_fdatasync);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_fsync(int *entry_lsn, unsigned int  fd)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_fsync\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_fsync);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_fsync(int *entry_lsn,  long  ret, unsigned int  fd)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_fsync\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_fsync);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_ftruncate(int *entry_lsn, unsigned int  fd, unsigned long  length)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_ftruncate\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_ftruncate);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    log_add_int(length);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_ftruncate(int *entry_lsn,  long  ret, unsigned int  fd, unsigned long  length)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_ftruncate\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_ftruncate);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_link(int *entry_lsn, const char  __user * oldname, const char  __user * newname)
{
    unsigned long irqflags;

    char* oldname_tmp = ERR_PTR(-ENOMEM);
    char* newname_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         oldname_tmp = ekm_getname(oldname);
         if(IS_ERR(oldname_tmp)) goto out;
    
         newname_tmp = ekm_getname(newname);
         if(IS_ERR(newname_tmp)) goto out;
    
    if(!file_traced(oldname)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_link\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_link);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(oldname_tmp);
    log_add_str(newname_tmp);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(oldname_tmp)) ekm_putname(oldname_tmp);
;
    if(!IS_ERR(newname_tmp)) ekm_putname(newname_tmp);
;
    enable_choose();
}
    
void ekm_exit_link(int *entry_lsn,  long  ret, const char  __user * oldname, const char  __user * newname)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_link\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_link);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_llseek(int *entry_lsn, unsigned int  fd, unsigned long  offset_high, unsigned long  offset_low,  loff_t  __user * result, unsigned int  origin)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_llseek\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_llseek);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    log_add_int(offset_high);
    log_add_int(offset_low);
    log_add_int(origin);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_llseek(int *entry_lsn,  long  ret, unsigned int  fd, unsigned long  offset_high, unsigned long  offset_low,  loff_t  __user * result, unsigned int  origin)
{
    unsigned long irqflags;

     loff_t *  result_tmp = NULL;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    
         result_tmp = ( loff_t * )ekm_kmalloc(sizeof result_tmp[0], GFP_KERNEL); 
         if(result_tmp == NULL) goto out; 
         if(copy_from_user(result_tmp, result, sizeof result_tmp[0])) goto out;
    
    ;

    EKM_PRINTK("EKM: exit sys_llseek\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_llseek);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_buf((char*)result_tmp, sizeof result_tmp[0]);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(result_tmp != NULL) ekm_kfree(result_tmp);
    enable_choose();
}
    
void ekm_enter_lseek(int *entry_lsn, unsigned int  fd, off_t  offset, unsigned int  origin)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_lseek\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_lseek);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    log_add_int(offset);
    log_add_int(origin);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_lseek(int *entry_lsn,  off_t  ret, unsigned int  fd, off_t  offset, unsigned int  origin)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_lseek\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_lseek);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_mkdir(int *entry_lsn, const char  __user * pathname, int  mode)
{
    unsigned long irqflags;

    char* pathname_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         pathname_tmp = ekm_getname(pathname);
         if(IS_ERR(pathname_tmp)) goto out;
    
    if(!file_traced(pathname)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_mkdir\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_mkdir);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(pathname_tmp);
    log_add_int(mode);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(pathname_tmp)) ekm_putname(pathname_tmp);
;
    enable_choose();
}
    
void ekm_exit_mkdir(int *entry_lsn,  long  ret, const char  __user * pathname, int  mode)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_mkdir\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_mkdir);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_open(int *entry_lsn, const char  __user * filename, int  flags, int  mode)
{
    unsigned long irqflags;

    char* filename_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         filename_tmp = ekm_getname(filename);
         if(IS_ERR(filename_tmp)) goto out;
    
    if(!file_traced(filename)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_open\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_open);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(filename_tmp);
    log_add_int(flags);
    log_add_int(mode);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(filename_tmp)) ekm_putname(filename_tmp);
;
    enable_choose();
}
    
void ekm_exit_open(int *entry_lsn,  long  ret, const char  __user * filename, int  flags, int  mode)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_open\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_open);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_pwrite64(int *entry_lsn, unsigned int  fd,  const char  __user * buf, size_t  count, loff_t  pos)
{
    unsigned long irqflags;

    char* buf_tmp = NULL;;

    ;
    
    disable_choose();

    
         buf_tmp = (char*)ekm_kmalloc(count, GFP_KERNEL); 
         if(buf_tmp == NULL) goto out; 
         if(copy_from_user(buf_tmp, buf, count)) goto out;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_pwrite64\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_pwrite64);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    log_add_sized_buf((char*)buf_tmp, count);
    log_add_int(count);
    log_add_int8(pos);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(buf_tmp != NULL) ekm_kfree(buf_tmp);
    enable_choose();
}
    
void ekm_exit_pwrite64(int *entry_lsn,  ssize_t  ret, unsigned int  fd,  const char  __user * buf, size_t  count, loff_t  pos)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_pwrite64\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_pwrite64);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_read(int *entry_lsn, unsigned int  fd,    char  __user * buf, size_t  count)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_read\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_read);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    log_add_int(count);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_read(int *entry_lsn,  ssize_t  ret, unsigned int  fd,    char  __user * buf, size_t  count)
{
    unsigned long irqflags;

    char* buf_tmp = NULL;;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    
         buf_tmp = (char*)ekm_kmalloc(ret, GFP_KERNEL); 
         if(buf_tmp == NULL) goto out; 
         if(copy_from_user(buf_tmp, buf, ret)) goto out;
    
    ;

    EKM_PRINTK("EKM: exit sys_read\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_read);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_sized_buf((char*)buf_tmp, ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(buf_tmp != NULL) ekm_kfree(buf_tmp);
    enable_choose();
}
    
void ekm_enter_rename(int *entry_lsn, const char  __user * oldname, const char  __user * newname)
{
    unsigned long irqflags;

    char* oldname_tmp = ERR_PTR(-ENOMEM);
    char* newname_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         oldname_tmp = ekm_getname(oldname);
         if(IS_ERR(oldname_tmp)) goto out;
    
         newname_tmp = ekm_getname(newname);
         if(IS_ERR(newname_tmp)) goto out;
    
    if(!file_traced(oldname)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_rename\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_rename);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(oldname_tmp);
    log_add_str(newname_tmp);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(oldname_tmp)) ekm_putname(oldname_tmp);
;
    if(!IS_ERR(newname_tmp)) ekm_putname(newname_tmp);
;
    enable_choose();
}
    
void ekm_exit_rename(int *entry_lsn,  long  ret, const char  __user * oldname, const char  __user * newname)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_rename\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_rename);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_rmdir(int *entry_lsn, const char  __user * pathname)
{
    unsigned long irqflags;

    char* pathname_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         pathname_tmp = ekm_getname(pathname);
         if(IS_ERR(pathname_tmp)) goto out;
    
    if(!file_traced(pathname)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_rmdir\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_rmdir);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(pathname_tmp);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(pathname_tmp)) ekm_putname(pathname_tmp);
;
    enable_choose();
}
    
void ekm_exit_rmdir(int *entry_lsn,  long  ret, const char  __user * pathname)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_rmdir\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_rmdir);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_symlink(int *entry_lsn, const char  __user * oldname, const char  __user * newname)
{
    unsigned long irqflags;

    char* oldname_tmp = ERR_PTR(-ENOMEM);
    char* newname_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         oldname_tmp = ekm_getname(oldname);
         if(IS_ERR(oldname_tmp)) goto out;
    
         newname_tmp = ekm_getname(newname);
         if(IS_ERR(newname_tmp)) goto out;
    
    if(!file_traced(oldname)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_symlink\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_symlink);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(oldname_tmp);
    log_add_str(newname_tmp);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(oldname_tmp)) ekm_putname(oldname_tmp);
;
    if(!IS_ERR(newname_tmp)) ekm_putname(newname_tmp);
;
    enable_choose();
}
    
void ekm_exit_symlink(int *entry_lsn,  long  ret, const char  __user * oldname, const char  __user * newname)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_symlink\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_symlink);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_sync(int *entry_lsn)
{
    unsigned long irqflags;

    ;

    ;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: enter sys_sync\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_sync);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    ;
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_exit_sync(int *entry_lsn,  long  ret)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_sync\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_sync);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_truncate(int *entry_lsn, const char  __user * filename, unsigned long  length)
{
    unsigned long irqflags;

    char* filename_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         filename_tmp = ekm_getname(filename);
         if(IS_ERR(filename_tmp)) goto out;
    
    if(!file_traced(filename)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_truncate\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_truncate);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(filename_tmp);
    log_add_int(length);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(filename_tmp)) ekm_putname(filename_tmp);
;
    enable_choose();
}
    
void ekm_exit_truncate(int *entry_lsn,  long  ret, const char  __user * filename, unsigned long  length)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_truncate\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_truncate);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_unlink(int *entry_lsn, const char  __user * pathname)
{
    unsigned long irqflags;

    char* pathname_tmp = ERR_PTR(-ENOMEM);

    ;
    
    disable_choose();

    
         pathname_tmp = ekm_getname(pathname);
         if(IS_ERR(pathname_tmp)) goto out;
    
    if(!file_traced(pathname)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_unlink\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_unlink);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_str(pathname_tmp);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(!IS_ERR(pathname_tmp)) ekm_putname(pathname_tmp);
;
    enable_choose();
}
    
void ekm_exit_unlink(int *entry_lsn,  long  ret, const char  __user * pathname)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_unlink\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_unlink);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}
    
void ekm_enter_write(int *entry_lsn, unsigned int  fd,  const char  __user * buf, size_t  count)
{
    unsigned long irqflags;

    char* buf_tmp = NULL;;

    ;
    
    disable_choose();

    
         buf_tmp = (char*)ekm_kmalloc(count, GFP_KERNEL); 
         if(buf_tmp == NULL) goto out; 
         if(copy_from_user(buf_tmp, buf, count)) goto out;
    
    if(!fd_traced(fd)) {*entry_lsn=EXPLODE_INVALID_LSN; goto out;};

    EKM_PRINTK("EKM: enter sys_write\n");

    spin_lock_irqsave(&log_lock, irqflags);
    *entry_lsn = ekm_lsn; /* set entry_lsn */;
    log_add_int(EKM_enter_write);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(fd);
    log_add_sized_buf((char*)buf_tmp, count);
    log_add_int(count);
    ;

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    if(buf_tmp != NULL) ekm_kfree(buf_tmp);
    enable_choose();
}
    
void ekm_exit_write(int *entry_lsn,  ssize_t  ret, unsigned int  fd,  const char  __user * buf, size_t  count)
{
    unsigned long irqflags;

    ;

    
    /* check if we have logged the entry of this system call */
    if(*entry_lsn == EXPLODE_INVALID_LSN)
        return;
    
    disable_choose();

    ;
    
    ;

    EKM_PRINTK("EKM: exit sys_write\n");

    spin_lock_irqsave(&log_lock, irqflags);
    ;
    log_add_int(EKM_exit_write);
    log_add_int(ekm_lsn ++);
    log_add_int(current->pid);
    log_add_int(EKM_LOG_MAGIC);

    log_add_int(ret);
    log_add_int(*entry_lsn);

    spin_unlock_irqrestore(&log_lock, irqflags);

    goto out; /* nuke stupid gcc warnings */
    
  out:
    ;
    enable_choose();
}

static void ekm_syscall_log_insall(void)
{
    ek_hooks.enter_sys_chdir = ekm_enter_chdir;
                  ek_hooks.exit_sys_chdir = ekm_exit_chdir;
    ek_hooks.enter_sys_close = ekm_enter_close;
                  ek_hooks.exit_sys_close = ekm_exit_close;
    ek_hooks.enter_sys_fdatasync = ekm_enter_fdatasync;
                  ek_hooks.exit_sys_fdatasync = ekm_exit_fdatasync;
    ek_hooks.enter_sys_fsync = ekm_enter_fsync;
                  ek_hooks.exit_sys_fsync = ekm_exit_fsync;
    ek_hooks.enter_sys_ftruncate = ekm_enter_ftruncate;
                  ek_hooks.exit_sys_ftruncate = ekm_exit_ftruncate;
    ek_hooks.enter_sys_link = ekm_enter_link;
                  ek_hooks.exit_sys_link = ekm_exit_link;
    ek_hooks.enter_sys_llseek = ekm_enter_llseek;
                  ek_hooks.exit_sys_llseek = ekm_exit_llseek;
    ek_hooks.enter_sys_lseek = ekm_enter_lseek;
                  ek_hooks.exit_sys_lseek = ekm_exit_lseek;
    ek_hooks.enter_sys_mkdir = ekm_enter_mkdir;
                  ek_hooks.exit_sys_mkdir = ekm_exit_mkdir;
    ek_hooks.enter_sys_open = ekm_enter_open;
                  ek_hooks.exit_sys_open = ekm_exit_open;
    ek_hooks.enter_sys_pwrite64 = ekm_enter_pwrite64;
                  ek_hooks.exit_sys_pwrite64 = ekm_exit_pwrite64;
    ek_hooks.enter_sys_read = ekm_enter_read;
                  ek_hooks.exit_sys_read = ekm_exit_read;
    ek_hooks.enter_sys_rename = ekm_enter_rename;
                  ek_hooks.exit_sys_rename = ekm_exit_rename;
    ek_hooks.enter_sys_rmdir = ekm_enter_rmdir;
                  ek_hooks.exit_sys_rmdir = ekm_exit_rmdir;
    ek_hooks.enter_sys_symlink = ekm_enter_symlink;
                  ek_hooks.exit_sys_symlink = ekm_exit_symlink;
    ek_hooks.enter_sys_sync = ekm_enter_sync;
                  ek_hooks.exit_sys_sync = ekm_exit_sync;
    ek_hooks.enter_sys_truncate = ekm_enter_truncate;
                  ek_hooks.exit_sys_truncate = ekm_exit_truncate;
    ek_hooks.enter_sys_unlink = ekm_enter_unlink;
                  ek_hooks.exit_sys_unlink = ekm_exit_unlink;
    ek_hooks.enter_sys_write = ekm_enter_write;
                  ek_hooks.exit_sys_write = ekm_exit_write;
}

static void ekm_syscall_log_uninstall(void)
{
    ek_hooks.enter_sys_chdir = NULL;
                  ek_hooks.exit_sys_chdir = NULL;
    ek_hooks.enter_sys_close = NULL;
                  ek_hooks.exit_sys_close = NULL;
    ek_hooks.enter_sys_fdatasync = NULL;
                  ek_hooks.exit_sys_fdatasync = NULL;
    ek_hooks.enter_sys_fsync = NULL;
                  ek_hooks.exit_sys_fsync = NULL;
    ek_hooks.enter_sys_ftruncate = NULL;
                  ek_hooks.exit_sys_ftruncate = NULL;
    ek_hooks.enter_sys_link = NULL;
                  ek_hooks.exit_sys_link = NULL;
    ek_hooks.enter_sys_llseek = NULL;
                  ek_hooks.exit_sys_llseek = NULL;
    ek_hooks.enter_sys_lseek = NULL;
                  ek_hooks.exit_sys_lseek = NULL;
    ek_hooks.enter_sys_mkdir = NULL;
                  ek_hooks.exit_sys_mkdir = NULL;
    ek_hooks.enter_sys_open = NULL;
                  ek_hooks.exit_sys_open = NULL;
    ek_hooks.enter_sys_pwrite64 = NULL;
                  ek_hooks.exit_sys_pwrite64 = NULL;
    ek_hooks.enter_sys_read = NULL;
                  ek_hooks.exit_sys_read = NULL;
    ek_hooks.enter_sys_rename = NULL;
                  ek_hooks.exit_sys_rename = NULL;
    ek_hooks.enter_sys_rmdir = NULL;
                  ek_hooks.exit_sys_rmdir = NULL;
    ek_hooks.enter_sys_symlink = NULL;
                  ek_hooks.exit_sys_symlink = NULL;
    ek_hooks.enter_sys_sync = NULL;
                  ek_hooks.exit_sys_sync = NULL;
    ek_hooks.enter_sys_truncate = NULL;
                  ek_hooks.exit_sys_truncate = NULL;
    ek_hooks.enter_sys_unlink = NULL;
                  ek_hooks.exit_sys_unlink = NULL;
    ek_hooks.enter_sys_write = NULL;
                  ek_hooks.exit_sys_write = NULL;
}
/* END SYSCALL*/
