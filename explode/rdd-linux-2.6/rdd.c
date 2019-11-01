/*
 * 2008 --- Modified by Can Sar (csar@cs.stanford.edu) and Junfeng Yang
 * (junfeng@cs.columbia.edu) to make it work with eXplode.
 */

/*
 * A sample, extra-simple block driver.
 *
 * Copyright 2003 Eklektix, Inc.  Redistributable under the terms
 * of the GNU GPL.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/explode.h>
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#include "rdd.h"

MODULE_LICENSE("Dual BSD/GPL");

int major_num = 0;
module_param(major_num, int, 0);
static int hardsect_size = 512;  /* Hardsector Size */
module_param(hardsect_size, int, 0);
static int nsectors = 1024;  /* How big the drive is */
module_param(nsectors, int, 0);
static int ndisks = 1;  /* Number of Disks */
module_param(ndisks, int, 0);
static int npartitions = 1;  /* Number of partitions per disk.  */
/* module_param(npartitions, int, 0); */ /* don't allow user to change this */
static int rdd_debug;
module_param(rdd_debug, int, 0);

/* How big the spare drive is in bytes */
static int rdd_size; 

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE 512


struct rdd_device {
        unsigned long size;
        spinlock_t lock;
        char **data;
        int changed;
        struct gendisk *gd;
};

static struct rdd_device *Devices;

/* this is a temporary buffer that we can use to copy disks */
static char **spare_disk;


/*this semaphore protces this temporary buffer */
static DECLARE_MUTEX(disk_copy_sem);

#define rdd_roundup(a, n) (((a) + ((n) - 1)) / (n))

static void *rdd_malloc(long size) {

        unsigned long num_dpages = rdd_roundup(size ,PAGE_SIZE);
        unsigned long num_ppages = rdd_roundup(num_dpages, PAGE_SIZE /  sizeof(char *));
        char **disk;
        int i;

	printk("allocate %lu p pages, size is %ld\n", num_ppages, size);
        disk = vmalloc(num_ppages*PAGE_SIZE);
        if(!disk)
                return NULL;

	printk("allocate %lu d pages\n", num_dpages);
        for(i = 0; i < num_dpages; i++) {
                disk[i] = (char *) __get_free_page(GFP_KERNEL);
                if(!disk[i]) {
                        int j;
                        for(j=0; j<i; j++) {
			        free_page((unsigned long)disk[j]);	
                        }
                        vfree(disk);
                        return NULL;
                }
        }
  

        return disk;
}

static void rdd_free(char **data, unsigned long size) {
        unsigned long num_dpages = rdd_roundup(size, PAGE_SIZE);
        /*unsigned long num_ppages = rdd_roundup(num_dpages, PAGE_SIZE /  sizeof(char *));*/
        int i;

        if(!data)
                return;

        for(i = 0; i < num_dpages; i++) {
                if(data[i])
                        free_page((unsigned long)  data[i]);
        }
        vfree(data);
}

static void rdddiskcpy(char **dest, char **src, unsigned long size)
{
        unsigned long num_dpages = rdd_roundup(size, PAGE_SIZE);
        int i;
        int length;

        for(i = 0; i < num_dpages; i++) {
                length = size % PAGE_SIZE ? size % PAGE_SIZE : PAGE_SIZE;
                memcpy(dest[i], src[i], length);
                size -= length;
        }
}

static int rddusercpy(char * arg, char **disk, unsigned long size, int write)
{
        unsigned long num_dpages = rdd_roundup(size, PAGE_SIZE);
        int i, ret;
        unsigned int length;
        unsigned count = 0;

        for(i = 0; i < num_dpages; i++) {
                length = size % PAGE_SIZE ? size % PAGE_SIZE : PAGE_SIZE;

                if(write)
                        ret = copy_to_user(arg + count, disk[i], length);
                else
                        ret = copy_from_user(disk[i], arg + count, length);

                if(ret)
                        return ret;

                size -= length;
                count += length;
        }

        return 0;
}
/*
 * Functions relating to the actual driver interface, that the FS deals with.
 */

/*
 * Handle an I/O transfer.
 */
static void rdd_transfer(struct rdd_device *dev, unsigned long sector,
                         unsigned long nsect, char *buffer, int write)
{
        unsigned long offset = sector*hardsect_size;
        unsigned long nbytes = nsect*hardsect_size;
        unsigned long count = 0;

        if ((offset + nbytes) > dev->size) {
                printk (KERN_NOTICE "rdd: Beyond-end write (%ld %ld)\n", offset, nbytes);
                return;
        }

        while (nbytes > 0) {
                int page = offset / PAGE_SIZE; /* Page to write */
                int ioffset = offset % PAGE_SIZE; 
                int length = min(nbytes, PAGE_SIZE - ioffset);
      
                if(write)
                        memcpy(dev->data[page] + ioffset, buffer + count, length);
                else
                        memcpy(buffer + count, dev->data[page] + ioffset,  length);

                count += length;
                offset += length;
                nbytes -= length;
        }

}

/*
 * Handle an I/O request.
 */
extern void (*explode_req_write)(struct request *req);
static void rdd_request(request_queue_t *q)
{
        struct request *req;
        struct rdd_device *device = q->queuedata;

        while ((req = elv_next_request(q)) != NULL) {
                if (! blk_fs_request(req)) {
                        printk (KERN_NOTICE "Skip non-CMD request\n");
                        end_request(req, 0);
                        continue;
                }
#if 0
                /*if(rdd_debug >= 2 && device->trace_is_on)*/ 
                if(rq_data_dir(req)) {
                        printk("RDD%d: ", (int)(device - Devices));
                        printk("%s", rq_data_dir(req)?"write":"read");
                        printk(" Sector %llu", (unsigned long long)req->sector);
                        printk(" Numsectors %u\n", req->current_nr_sectors);
                }
#endif

#if 0
                // FIXME.  this should be in generic_make_request so we
                // can fail any disk request
                if(fail_req(req)) {
                        printk("RDD: failing %s request to sector %llu, %d\n", 
                               rq_data_dir(req)?"write":"read",
                               (unsigned long long)(req->sector),
                               req->current_nr_sectors);
                        end_request(req, 0);
                        continue;
                }
#endif


#if 0
                if(rq_data_dir(req) == 0) {
                        // fail read
#if 0
                        printk("RDD: read\n");
                        dump_stack();
#endif
                        if(choose(EKM_DISK_READ 3, 2)) {
                                end_request(req, 0);
                                continue;
                        }
                }
#endif

                rdd_transfer(device, req->sector, req->current_nr_sectors,
                             req->buffer, rq_data_dir(req));
                if(rq_data_dir(req))
                        EXPLODE_HOOK(req_write, req);
                else
                        EXPLODE_HOOK(req_read, req);
                end_request(req, 1);
        }
}


/*
 * Ioctl.
 */
int rdd_ioctl (struct inode *inode, struct file *filp,
               unsigned int cmd, unsigned long arg)
{
	struct hd_geometry geo;
	struct rdd_device *Device;
	
	switch(cmd) {
                int size;
                /*
                 * We have no real geometry, of course, so make something up.
                 */
	case HDIO_GETGEO:
                Device = &Devices[iminor(inode)/npartitions];
                size = Device->size*(hardsect_size/KERNEL_SECTOR_SIZE);
                geo.cylinders = (size & ~0x3f) >> 6;
                geo.heads = 4;
                geo.sectors = 16;
                geo.start = 4;
                if (copy_to_user((void *) arg, &geo, sizeof(geo)))
                        return -EFAULT;
                return 0;
                /* Custom Commands */
	case RDD_GET_DISK: {
                Device = &Devices[iminor(inode)/npartitions];
                if(!arg) return -EINVAL;
                down(&disk_copy_sem);
                /*
                 * acquire semaphore before spinlock so we don't sleep with lock held
                 */
                spin_lock(&Device->lock);
                rdddiskcpy(spare_disk, Device->data , Device->size);
                /*memcpy(spare_disk, Device->data , Device->size);*/
                spin_unlock(&Device->lock);
                /*if (copy_to_user((char *)arg, spare_disk, Device->size)) {*/
                if (rddusercpy((char *) arg, spare_disk, Device->size, 1)) {
                        up(&disk_copy_sem);
                        return -EFAULT;
                }
                up(&disk_copy_sem);
                return 0;
	}
	case RDD_SET_DISK: {
                Device = &Devices[iminor(inode)/npartitions];
                if(!arg) return -EINVAL;
                down(&disk_copy_sem);
                /*if (copy_from_user(spare_disk, (char *) arg, Device->size)) {*/
                if (rddusercpy((char *) arg, spare_disk, Device->size, 0)) {
                        up(&disk_copy_sem);
                        return -EFAULT;
                }
                spin_lock(&Device->lock);
                rdddiskcpy(Device->data, spare_disk, Device->size);
                /*memcpy(Device->data, spare_disk, Device->size);*/
                Device->changed = 1;
                spin_unlock(&Device->lock);
                up(&disk_copy_sem);
                return 0;
	}
	case RDD_COPY_DISK: {
                Device = &Devices[iminor(inode)/npartitions]; /* src disk */
                if (arg < 0 || arg >= ndisks)
                        return -EFAULT;
                /*memcpy(Devices[arg].data, Device->data , Device->size);*/
                Devices[arg].changed = 1;
                rdddiskcpy(Devices[arg].data, Device->data , Device->size);
                return 0;
	}
        }

        return -ENOTTY; /* unknown command */
}

int rdd_open(struct inode *inode, struct file *file)
{
	check_disk_change(inode->i_bdev);
	return 0;
}

static int rdd_media_changed(struct gendisk *disk)
{
	struct rdd_device *dev = disk->private_data;
	int changed = 0;
	if(dev < Devices || dev >= Devices + ndisks)
		panic("gendisk isn't rdd!");
	spin_lock(&dev->lock);
	if(dev->changed) {
		changed = 1;
		dev->changed = 0;
	}
	spin_unlock(&dev->lock);
	return changed;
}

/*
 * The device operations structure.
 */
static struct block_device_operations rdd_ops = {
        .owner           = THIS_MODULE,
        .open            = rdd_open,
        .ioctl	     = rdd_ioctl,
        .media_changed   = rdd_media_changed,
};

static int rdd_init_dev(struct rdd_device* dev)
{
        int minor;
	struct gendisk *gd;

        dev->size = nsectors*hardsect_size;
        spin_lock_init(&dev->lock);
        
        // allocate ram for disk
        dev->data = rdd_malloc(dev->size);
        if(dev->data == NULL)
                goto out;
        
        // allocate gd
        gd = dev->gd = alloc_disk(npartitions);
        if(!gd)
                goto out_data;

        gd->queue = blk_init_queue(rdd_request, &dev->lock);
        if(!gd->queue)
                goto out_gd;

        blk_queue_hardsect_size(gd->queue, hardsect_size);
        gd->queue->queuedata = dev;

        gd->major = major_num;
        gd->first_minor = minor = dev - Devices;
        gd->fops = &rdd_ops;
        gd->private_data = dev;
        sprintf (gd->disk_name, "rdd%d", minor);
        set_capacity(gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
        add_disk(gd);

        return 0;
        
 out_gd:   
        del_gendisk(dev->gd);
        put_disk(dev->gd);

     
 out_data:
        rdd_free(dev->data, dev->size);

 out:
        return -ENOMEM;
}

static void rdd_cleanup_dev(struct rdd_device *dev)
{
        blk_cleanup_queue(dev->gd->queue);
        
        del_gendisk(dev->gd);
        put_disk(dev->gd);

        rdd_free(dev->data, dev->size);
}


static int __init rdd_init(void)
{
        int i=0, j=0;
        int error = -EINVAL;
  
        /*
         * Sanity check on the parameters
         */
        if(ndisks==0) {
                printk(KERN_WARNING "invalid module parameters: ndisks=%d\n", ndisks);
                goto out;
        }

        if(nsectors==0) {
                printk(KERN_WARNING "invalid module parameters: nsectors=%d\n", nsectors);
                goto out;
        }
        rdd_size = nsectors*hardsect_size;

        /*
         * Get registered.
         */
        major_num = register_blkdev(major_num, "rdd");
        if (major_num <= 0) {
                printk(KERN_WARNING "rdd: unable to get major number\n");
                error = major_num;
                goto out;
        }


        /*
         * Set up our internal device.
         */
        error = -ENOMEM;
        Devices = vmalloc(ndisks * sizeof(struct rdd_device));
        if(!Devices)
                goto out_register;


        /*
         * Allocate our temporary disk for copying.
         */
        spare_disk = rdd_malloc(rdd_size);
        if(!spare_disk)
                goto out_device;

        for(i=0; i<ndisks; i++)
                if(rdd_init_dev(&Devices[i]) < 0)
                        goto out_init_dev;

        return 0;


 out_init_dev:
        for(j=0; j<i; j++)
                rdd_cleanup_dev(&Devices[j]);
        rdd_free(spare_disk, rdd_size);

 out_device:
        vfree(Devices);

 out_register:
        unregister_blkdev(major_num, "rdd");

 out:
        return error;
}

static void __exit rdd_exit(void)
{
        int i;

        for(i = 0; i < ndisks; i++)
                rdd_cleanup_dev(&Devices[i]);

        rdd_free(spare_disk, rdd_size);

        vfree(Devices);

        unregister_blkdev(major_num, "rdd");
}

module_init(rdd_init); 
module_exit(rdd_exit);
