#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bio.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/linker.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mdioctl.h>
#include <sys/mutex.h>
#include <sys/sx.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/devicestat.h>
#include <sys/stat.h>
#include <sys/syscallsubr.h>
#include <fs/devfs/devfs_int.h>
  
#include <geom/geom.h>

#include <sys/explode.h>

#define __KERNEL__
 
#include "rdd.h"
 
#define MD_MODVER 1
 
#define MD_SHUTDOWN 0x10000     /* Tell worker thread to terminate. */
 
static MALLOC_DEFINE(M_MD, "MD disk", "Memory Disk");
static MALLOC_DEFINE(M_MDSECT, "MD sectors", "Memory Disk Sectors");

static MALLOC_DEFINE(M_MDPATH, "MD Path", "MD Path");
 

static int md_debug;
SYSCTL_INT(_debug, OID_AUTO, mddebug, CTLFLAG_RW, &md_debug, 0, "");

static g_init_t g_md_init;
static g_fini_t g_md_fini;
static g_start_t g_md_start;
static g_access_t g_md_access;
static g_ctl_destroy_geom_t g_md_destroy;
static g_ioctl_t g_ioctl;

//static int      mdunits;
//static struct cdev *status_dev = 0;
static struct sx md_sx;

/*static d_ioctl_t mdctlioctl;

static struct cdevsw mdctl_cdevsw = {
        .d_version =    D_VERSION,
        .d_ioctl =      mdctlioctl,
        .d_name =       "RDD",
	};*/

struct g_class g_md_class = {
        .name = "RDD",
        .version = G_VERSION,
        .init = g_md_init,
        .fini = g_md_fini,
        .start = g_md_start,
        .access = g_md_access,
	.destroy_geom = g_md_destroy,
	.ioctl = g_ioctl,
};

DECLARE_GEOM_CLASS(g_md_class, g_rdd);


static LIST_HEAD(, md_s) md_softc_list = LIST_HEAD_INITIALIZER(&md_softc_list);

struct md_s {
        int unit;
        LIST_ENTRY(md_s) list;
        struct bio_queue_head bio_queue;
        struct mtx queue_mtx;
        struct cdev *dev;
        off_t mediasize;
        unsigned sectorsize;
        unsigned opencount;
        unsigned fwheads;
        unsigned fwsectors;
        unsigned flags;
        char name[20];
        struct proc *procp;
        struct g_geom *gp;
        struct g_provider *pp;
        int (*start)(struct md_s *sc, struct bio *bp);

        /* MD_PRELOAD related fields */
        u_char *pl_ptr;
        size_t pl_len;
};

static int
g_md_access(struct g_provider *pp, int r, int w, int e)
{
        struct md_s *sc;

        sc = pp->geom->softc;
        if (sc == NULL)
                return (ENXIO);
        r += pp->acr;
        w += pp->acw;
        e += pp->ace;
        if ((sc->flags & MD_READONLY) != 0 && w > 0)
                return (EROFS);
        if ((pp->acr + pp->acw + pp->ace) == 0 && (r + w + e) > 0) {
                sc->opencount = 1;
        } else if ((pp->acr + pp->acw + pp->ace) > 0 && (r + w + e) == 0) {
                sc->opencount = 0;
        }
        return (0);
}

#include <sys/kdb.h>
static void
g_md_start(struct bio *bp)
{
        struct md_s *sc;

        sc = bp->bio_to->geom->softc;
	//printf("Index %d\n", bp->bio_from->stat->unit_number);
        mtx_lock(&sc->queue_mtx);
        bioq_disksort(&sc->bio_queue, bp);
	
        mtx_unlock(&sc->queue_mtx);
        wakeup(sc);


	//printf("g_md_start done\n");
}


static int
mdstart_preload(struct md_s *sc, struct bio *bp)
{
  //if(bp->bio_cmd) 
  //printf("Dev %p\n", bp->bio_dev);

  //bp->bio_dev = sc->dev;

  switch (bp->bio_cmd) {
        case BIO_READ:
                bcopy(sc->pl_ptr + bp->bio_offset, bp->bio_data,
                    bp->bio_length);
		EXPLODE_HOOK(req_read, bp);
		//printf("Read ");
                break;
        case BIO_WRITE:
                bcopy(bp->bio_data, sc->pl_ptr + bp->bio_offset,
                    bp->bio_length);
		EXPLODE_HOOK(req_write, bp);
		{
			struct bio *bio = bp;
			int dev = dev2udev(bio->bio_dev);
			printf("bio_start_write dev = %d\n", dev);
		}

		printf("Write ");
		printf("Offset %d Length %d\n", (int) bp->bio_offset/512, (int) bp->bio_length);
                break;
        }
        bp->bio_resid = 0;
        return (0);
}

static void
md_kthread(void *arg)
{
        struct md_s *sc;
        struct bio *bp;
        int error;

        sc = arg;
        mtx_lock_spin(&sched_lock);
        sched_prio(curthread, PRIBIO);
        mtx_unlock_spin(&sched_lock);

        for (;;) {
	  if (sc->flags & MD_SHUTDOWN) {
                        sc->procp = NULL;
                        wakeup(&sc->procp);
			kthread_exit(0);
	  }
                mtx_lock(&sc->queue_mtx);
                bp = bioq_takefirst(&sc->bio_queue);
                if (!bp) {
                        msleep(sc, &sc->queue_mtx, PRIBIO | PDROP, "mdwait", 0);
                        continue;
                }
                mtx_unlock(&sc->queue_mtx);
                if (bp->bio_cmd == BIO_GETATTR) {
                        if (sc->fwsectors && sc->fwheads &&
                            (g_handleattr_int(bp, "GEOM::fwsectors",
                            sc->fwsectors) ||
                            g_handleattr_int(bp, "GEOM::fwheads",
                            sc->fwheads)))
                                error = -1;
                        else
                                error = EOPNOTSUPP;
                } else {
                        error = sc->start(sc, bp);
                }

                if (error != -1) {
                        bp->bio_completed = bp->bio_length;
                        g_io_deliver(bp, error);
                }
        }
}

static struct md_s *
mdnew(int unit, int *errp)
{
        struct md_s *sc, *sc2;
        int error, max = -1;

        *errp = 0;
        LIST_FOREACH(sc2, &md_softc_list, list) {
                if (unit == sc2->unit) {
                        *errp = EBUSY;
                        return (NULL);
                }
                if (unit == -1 && sc2->unit > max) 
                        max = sc2->unit;
        }
        if (unit == -1)
                unit = max + 1;
        sc = (struct md_s *)malloc(sizeof *sc, M_MD, M_WAITOK | M_ZERO);
        bioq_init(&sc->bio_queue);
        mtx_init(&sc->queue_mtx, "md bio queue", NULL, MTX_DEF);
        sc->unit = unit;
        sprintf(sc->name, "rdd%d", unit);
        LIST_INSERT_HEAD(&md_softc_list, sc, list);
        error = kthread_create(md_kthread, sc, &sc->procp, 0, 0,"%s", sc->name);
        if (error == 0)
                return (sc);
        LIST_REMOVE(sc, list);
        mtx_destroy(&sc->queue_mtx);
	free(sc, M_MD);
        *errp = error;
        return (NULL);
}


extern TAILQ_HEAD(,cdev_priv) cdevp_list;

static void
mdinit(struct md_s *sc)
{

        struct g_geom *gp;
        struct g_provider *pp;
	//struct stat sb;
	//char *path;

	//printf("mdinit %p\n", sc);

        g_topology_lock();
        gp = g_new_geomf(&g_md_class, "rdd%d", sc->unit);
	//printf("dev %d\n", gp->
        gp->softc = sc;

	//sc->dev = (struct cdev *)(81+sc->unit);
        pp = g_new_providerf(gp, "rdd%d", sc->unit);
	//path = (char *)malloc(10, M_MDPATH, M_WAITOK | M_ZERO);
	//strcpy(path, "rdd0");
	//printf("Name %s\n", path);
	//kern_stat(curthread, path, UIO_SYSSPACE, &sb);
	//free(path, M_MDPATH);
	//printf("DEVID %d\n", (int) sb.st_rdev);
	//printf("DevNum %d\n", pp->stat->device_number);

        pp->mediasize = sc->mediasize;
        pp->sectorsize = sc->sectorsize;
        sc->gp = gp;
        sc->pp = pp;
        g_error_provider(pp, 0);
        g_topology_unlock();

	struct cdev_priv *cdp;

	dev_lock();
	TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
	  if(strcmp(gp->name ,cdp->cdp_c.si_name) == 0) {
	    sc->dev = &(cdp->cdp_c);
	    printf("ID %d\n", dev2udev(sc->dev));
	  }
	}
	dev_unlock();

	//printf("mdinit end\n");
}

static int
mddestroy(struct md_s *sc, struct thread *td)
{
  //printf("mddestroy\n");
        if (sc->gp) {
                sc->gp->softc = NULL;
                g_topology_lock();
                g_wither_geom(sc->gp, ENXIO);
                g_topology_unlock();
                sc->gp = NULL;
                sc->pp = NULL;
        }
	//printf("past this\n");
        sc->flags |= MD_SHUTDOWN;
	//printf("waking up\n");
        wakeup(sc);
	//printf("woke up\n");
        while (sc->procp != NULL)
                tsleep(&sc->procp, PRIBIO, "mddestroy", hz / 10);
	//printf("past loop\n");
        mtx_destroy(&sc->queue_mtx);
	//printf("destroyed mutex\n");

        LIST_REMOVE(sc, list);
	free(sc->pl_ptr, M_MDSECT);
        free(sc, M_MD);
	//printf("mddestroy done\n");
        return (0);
}

static int
g_md_destroy(struct gctl_req *req, struct g_class *cp, struct g_geom *gp)
{
  if(!gp->softc) {
    printf("Error: gp->softc == NULL\n");
    return -1;
  }

  g_topology_unlock();
  sx_xlock(&md_sx);
  mddestroy(gp->softc, 0);
  sx_xunlock(&md_sx);
  g_topology_lock();

  return 0;
}

static struct md_s *
mdfind(int unit)
{
        struct md_s *sc;

        LIST_FOREACH(sc, &md_softc_list, list) {
                if (sc->unit == unit)
                        break;
        }
        return (sc);
}

static int
xmdctlioctl(struct g_provider *pp, u_long cmd, caddr_t addr, int flags, struct thread *td)
{
        //struct md_s *sc;
        //int error;//, i;

        if (md_debug)
                printf("mdctlioctl(%s %lx %p %x %p)\n",
                        pp->name, cmd, addr, flags, td);

	struct md_s *sc;

        sc = pp->geom->softc;


        //error = 0;
        switch (cmd) {
	case RDD_GET_DISK: {
	  return copyout(sc->pl_ptr , *(char **) addr, sc->pl_len);
	}
	case RDD_SET_DISK: {
	  return copyin(*(char **) addr, sc->pl_ptr, sc->pl_len);
	}
	case RDD_COPY_DISK: {
	  struct md_s *sc_to = mdfind( *(int *) addr);
	  if(!sc_to)
	    return EINVAL;
	  memcpy(sc_to->pl_ptr, sc->pl_ptr, sc->pl_len);
	  return 0;
	}
#if 0
        case MDIOCDETACH:
                if (mdio->md_mediasize != 0 || mdio->md_options != 0)
                        return (EINVAL);

                sc = mdfind(mdio->md_unit);
                if (sc == NULL)
                        return (ENOENT);
                if (sc->opencount != 0 && !(sc->flags & MD_FORCE))
                        return (EBUSY);
                return (mddestroy(sc, td));
#endif
        default:
                return (ENOIOCTL);
        };
}

int g_ioctl(struct g_provider *pp, u_long cmd, void *addr, int flags, struct thread *td)
{
        int error; 

        sx_xlock(&md_sx);
        error = xmdctlioctl(pp, cmd, addr, flags, td);
        sx_xunlock(&md_sx);
        return (error);
}

static void
md_preloaded(u_char *image, size_t length)
{
        struct md_s *sc;
        int error;

        sc = mdnew(-1, &error);
        if (sc == NULL)
                return;

	//printf("Mediasize %d Sectorsize %d\n", length, DEV_BSIZE);

        sc->mediasize = length;
        sc->sectorsize = DEV_BSIZE;
        sc->pl_ptr = image;
        sc->pl_len = length;
        sc->start = mdstart_preload;

        mdinit(sc);
}

static void
g_md_init(struct g_class *mp __unused)
{
        u_char *ptr;
        unsigned len = hardsect_size * nsectors;
	int i;

        sx_init(&md_sx, "MD config lock");
        g_topology_unlock();

	for(i = 0; i < ndisks; i++) {
	  ptr = (u_char *) malloc(len, M_MDSECT, M_WAITOK);
	  
	  if(!ptr) {
	    printf("Couldn't allocate memory\n");
	    return;
	  }

	  sx_xlock(&md_sx);
	  md_preloaded(ptr, len);
	  sx_xunlock(&md_sx);
	}

        g_topology_lock();
}

static void
g_md_fini(struct g_class *mp __unused)
{

        sx_destroy(&md_sx);
}

