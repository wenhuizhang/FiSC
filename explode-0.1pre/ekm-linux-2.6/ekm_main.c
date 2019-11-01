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


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/explode.h>
#include <linux/fs.h>     /* everything... */
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/explode.h>

#include "ekm.h"
#include "ekm_log.h"
#include "ekm_thread.h"
#include "ekm_choice.h"

MODULE_LICENSE("Dual BSD/GPL");

static int major_num = 0;
static struct gendisk *gd;

static void install_hooks(void);
static void uninstall_hooks(void);

int ekm_ioctl (struct inode *inode, struct file *filp,
               unsigned int cmd, unsigned long arg)
{
        switch(cmd) {
        case EKM_START_TRACE: {
                struct ekm_start_trace_request req;
                if(copy_from_user(&req, (struct ekm_start_trace_request*)arg,
                                  sizeof req))
                        return -EFAULT;
                return start_trace(&req);
        }
        case EKM_STOP_TRACE: {
                return stop_trace((int) arg);
        }
                /*case EKM_SET_FS: {
                  struct ekm_setfs_request request;
    
                  if(!arg) return -EINVAL;
                  if(copy_from_user(&request,(struct ekm_setfs_request *) arg,
                  sizeof(struct ekm_setfs_request)))
                  return -EFAULT;
                  return ekm_set_fs(request.handle, request.fs);
                  }  */
        case EKM_BUFFER_LOG_SIZE: {
                return put_user(get_log_size(), (unsigned long __user *) arg);
        }
        case EKM_GET_BUFFER_LOG: {
                int ret, ret2;
                struct ekm_get_log_request request;
    
                if(!arg) return -EINVAL;
                if(copy_from_user(&request,(struct ekm_get_log_request *) arg,
                                  sizeof(struct ekm_get_log_request)))
                        return -EFAULT;
                ret = copy_log_buffer(request.buffer, &request.size);
                if(ret)
                        request.size = 0;
                ret2 = copy_to_user((struct ekm_get_log_request *) arg, &request,
                                    sizeof(struct ekm_get_log_request));
                if(ret2)
                        return -EFAULT;
                return ret;
        }
        case EKM_EMPTY_LOG: {
                empty_log();
                return 0;
        }
#ifdef ENABLE_EKM_RUN
        case EKM_RUN: {
                return ekm_run_thread((int) arg);
        }
        case EKM_RAN: {
                return ekm_ran_thread((int) arg);
        }
#endif
        case EKM_NO_RUN: {
                return ekm_norun_thread((int) arg);
        }
#ifdef ENABLE_SBD_GET_PID
        case SBD_GET_PID: {
                int ret;
                struct ekm_get_pid_req req;
                if(copy_from_user(&req, (struct ekm_get_pid_req *)arg, sizeof(req)))
                        return -EINVAL;

                if(ret = ekm_get_pid(&req))
                        return ret;

                if(copy_to_user((struct ekm_get_pid_req *) arg, &req, sizeof(req)))
                        return -EINVAL;
        }
#endif
        case EKM_SET_CHOICE: {
                struct ekm_choice choice;
                disable_choose();
                if(copy_from_user(&choice, (struct ekm_choice *) arg,
                                  sizeof choice)) {
                        enable_choose();
                        return -EFAULT; 
                }
                ekm_set_choice(&choice);
                enable_choose();
                return 0;
        }
        case EKM_GET_CHOICE: {
                struct ekm_choice choice;
                disable_choose();
                ekm_get_choice(&choice);
                if(copy_to_user((struct ekm_choice *) arg, &choice,
                                sizeof choice)) {
                        enable_choose();
                        return -EFAULT; 
                }
                enable_choose();
                return 0;
        }
	  
        case EKM_APP_RECORD: {
                struct ekm_app_record ar;
                char* data;
                EKM_PRINTK("ekm: EKM_APP_RECORD\n");
                if(copy_from_user((void*)&ar, (void *)arg, sizeof ar))
                        return -EFAULT;

                EKM_PRINTK("ekm: ar.type = %d\n", ar.type);

                if(EKM_CAT(ar.type) < EKM_USR_CAT_BEGIN
                   || EKM_CAT(ar.type) > EKM_USR_CAT_END)
                        return -EINVAL;
	  
                data = (char*)kmalloc(ar.len, GFP_USER);
                if(!data)
                        return -ENOMEM;
	  
                if(copy_from_user(data, ar.data, ar.len)) {
                        kfree(data);
                        return -EFAULT;
                }
                ar.data = data;

                EKM_PRINTK("ekm: copied app record from user\n");
	  
                ekm_log_app_record(&ar);
                kfree(data);
        }
                break;

        case EKM_KILL_DISK:
                ekm_kill_disk((u32)arg);
                break;
        case EKM_REVIVE_DISK:
                ekm_revive_disk((u32)arg);
                break;

        default:
                printk("EKM: Unrecognized command number %d\n", cmd);
                return -1;
        }
        return 0;
}

static struct block_device_operations ekm_ops = {
        .owner           = THIS_MODULE,
        .ioctl	     = ekm_ioctl,
};

static int __init ekm_init(void)
{

        // just in case there are dangling function pointers from prevous run
        uninstall_hooks();
        /*
         * Get registered.
         */
        major_num = register_blkdev(major_num, "ekm");
        if (major_num <= 0) {
                printk(KERN_WARNING "ekm: unable to get major number\n");
                return -1;
        }
        /*
         * And the gendisk structure.
         */
        gd = alloc_disk(1);
        if (!gd)
                goto out_unregister;
        gd->major = major_num;
        gd->first_minor = 0;
        gd->fops = &ekm_ops;
        gd->private_data = NULL;
        strcpy (gd->disk_name, "ekm");
        //strcpy (gd->devfs_name, "ekm");
        set_capacity(gd, 0);
        gd->queue = NULL;
        add_disk(gd);

        install_hooks();
        return 0;
 out_unregister:
        unregister_blkdev(major_num, "ekm");
        return -1;
}

static void __exit ekm_exit(void)
{
        stop_traces();
        uninstall_hooks();
        empty_log();

        del_gendisk(gd);
        put_disk(gd);
        unregister_blkdev(major_num, "ekm");
}

static void install_hooks(void)
{
        spin_lock(&ek_hooks_lock);
        ekm_choice_install();
        ekm_log_install();
        spin_unlock(&ek_hooks_lock);
}

static void uninstall_hooks(void)
{
        spin_lock(&ek_hooks_lock);
        ekm_choice_uninstall();
        ekm_log_uninstall();
        spin_unlock(&ek_hooks_lock);
}
	
module_init(ekm_init); 
module_exit(ekm_exit);
