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

#define BUFFERSIZE 256


static d_ioctl_t     ekm_ioctl;

/* Character device entry points */
static struct cdevsw echo_cdevsw = {
        .d_name = "ekm",
        .d_ioctl = ekm_ioctl,
        .d_version = D_VERSION
};


static struct cdev *echo_dev;

MALLOC_DECLARE(M_APPRECORD);
MALLOC_DEFINE(M_APPRECORD, "ekm_app_record", "ekm app specific record");

static int
ekm_ioctl(struct cdev *dev, u_long cmd, caddr_t arg,
          int flags, struct thread *td)
{

        switch(cmd) {  
        case EKM_START_TRACE: {
                struct ekm_start_trace_request req;
                if(copyin((struct ekm_start_trace_request *)arg, (void*)&req, sizeof req))
                        return EFAULT;
                return start_trace(req);
        }
        case EKM_STOP_TRACE: {
                return stop_trace(*(int *) arg);
        }
        case EKM_BUFFER_LOG_SIZE: {
                *(long *) arg = get_log_size();
                return 0;
        }
        case EKM_GET_BUFFER_LOG: {
                int ret;
                struct ekm_get_log_request request;
                request = *(struct ekm_get_log_request *) arg;
                if(copyin((struct ekm_get_log_request*)arg, (void*)request, sizeof request))
                        return EFAULT;
                ret = copy_log_buffer(request.buffer, &request.size);
                if(ret)
                        request.size = 0;
                if(copyout(&(((struct ekm_log_request *)arg)->size), request.size))
                        return EFAULT;
                return ret;
        }
        case EKM_EMPTY_LOG: {
                empty_log();
                return 0;
        }
#ifdef ENABLE_EKM_RUN
        case EKM_RAN: {
                return ekm_ran_thread((int) arg);
        }
#endif
        case EKM_APP_RECORD: {
                struct ekm_app_record ar;
                char* data;
                EKM_PRINTK("ekm: EKM_APP_RECORD\n");
                if(copyin((struct ekm_app_record*)arg, (void*)&ar, sizeof ar))
                        return EFAULT;
                
                EKM_PRINTK("ekm: ar.type = %d\n", ar.type);
          
                if(EKM_CAT(ar.type) < EKM_USR_CAT_BEGIN
                   || EKM_CAT(ar.type) > EKM_USR_CAT_END)
                        return EINVAL;
	  
                MALLOC(data, char*, ar.size, M_APPRECORD, M_WAITOK);
                if(!data)
                        return ENOMEM;

                if(copyin(ar->data, data, ar->len)) {
                        FREE(data, M_APPRECORD);
                        return EFAULT;
                }
                ar.data = data;

                EKM_PRINTK("ekm: copied app record from user\n");                
                ekm_log_app_record(&ar);
                FREE(g,M_APPRECORD);
        }
                break;

        default:
                printf("EKM: Unrecognized command number %lu\n", cmd);
                return ENOTTY;
        }

        return 0;
}

static void install_hooks(void)
{
        mtx_lock(&ek_hooks_mutex);
        ekm_log_install();
        mtx_unlock(&ek_hooks_mutex);
}

static void uninstall_hooks(void)
{
        mtx_lock(&ek_hooks_mutex);
        ekm_log_uninstall();
        mtx_unlock(&ek_hooks_mutex);
}

/*
 * This function is called by the kld[un]load(2) system calls to
 * determine what actions to take when a module is loaded or unloaded.
 */

extern struct mtx log_mutex;
static int
ekm_loader(struct module *m, int what, void *arg)
{
        int err = 0;

        switch (what) {
        case MOD_LOAD: {                /* kldload */
                mtx_init(&log_mutex, "ekm_log_mutex", NULL, MTX_DEF);
                install_hooks();
                echo_dev = make_dev(&echo_cdevsw,
                                    0,
                                    UID_ROOT,
                                    GID_WHEEL,
                                    0600,
                                    "ekm");
                /* kmalloc memory for use by this driver */
                //MALLOC(echomsg, t_echo *, sizeof(t_echo), M_ECHOBUF, M_WAITOK);
                printf("Ekm device loaded.\n");
                break;
        }
        case MOD_UNLOAD:
                printf("Ekm device unloading\n");
                destroy_dev(echo_dev);
                printf("Destroyed dev\n");
                uninstall_hooks();
                printf("Uninstalled hooks\n");
                empty_log();
                //FREE(echomsg,M_ECHOBUF);
                printf("Ekm device unloaded.\n");
                break;
        default:
                err = EINVAL;
                break;
        }
        return(err);
}

DEV_MODULE(ekm,ekm_loader,NULL);
