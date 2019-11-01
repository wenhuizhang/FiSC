/*
 *
 * Copyright (C) 2008 Junfeng Yang (junfeng@cs.columbia.edu)
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


#include <linux/config.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/mtd/compatmac.h>
#include <linux/mtd/mtd.h>

#include "smtd_log.h"
#include "../ekm/ekm.h"

#undef CONFIG_MTD_PARTITIONS
#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

/* for ioctl command code */
//#include "../rdd/rdd.h"

#define D(x...) printk(x)
//#define D(x...)

static struct nand_oobinfo nand_oob_16 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 6,
	.eccpos = {0, 1, 2, 3, 6, 7},
	.oobfree = { {8, 8} }
};

#define MAX_PAGE_SIZE (2048)

struct nandsim {
        struct mtd_info mtd;

#ifdef CONFIG_MTD_PARTITIONS
        struct mtd_partition part;
#endif
        u_char *mem; // holds data
        u_char *oob; // holds oob data

        u_char buf[MAX_PAGE_SIZE]; // internal buffer for partial read/write

        /* NAND flash "geometry" */
        struct nandsim_geometry {
                uint32_t totsz;     /* total flash size, bytes */
                uint32_t secsz;     /* flash sector (erase block) size, bytes */
                uint pgsz;          /* NAND flash page size, bytes */
                uint oobsz;         /* page OOB area size, bytes */
                
                /* the fields below are computed from the fields above */
                uint pgnum;         /* total number of pages */
                uint pgsec;         /* number of pages per sector */
                uint pgszoob;       /* page size including OOB , bytes*/
                uint secszoob;      /* sector size including OOB, bytes */
                uint32_t totoobsz;  /* total OOB area size, bytes */

                /* for div */
                uint pgshift;
                
        } geom;

        int index; // index of this
        int dev; // id for this mtd
};

static unsigned long totsz_in_kb = 8*1024; // total flash size in KiB
static unsigned long secsz_in_kb = 8; // erase block size in KiB
static unsigned long pgsz = 512; // page size in bytes
static unsigned long oobsz = 16; // page oob size in bytes

#ifdef MODULE
module_param(totsz_in_kb, ulong, 0);
MODULE_PARM_DESC(totsz_in_kb, "Total device size in KiB");
module_param(secsz_in_kb, ulong, 0);
MODULE_PARM_DESC(secsz_in_kb, "Device erase block size in KiB");
module_param(pgsz, ulong, 0);
MODULE_PARM_DESC(pgsz, "Device page size in bytes");
module_param(oobsz, ulong, 0);
MODULE_PARM_DESC(oobsz, "Device page oob size in bytes");
#endif

/* We could store these in the mtd structure, but we only support 1 device..
   static struct mtd_info *mtd_info; */
static struct nandsim *ns;

// computes offset of oob given virtual offset.
static loff_t oob_off(loff_t off)
{
        uint pg = off>>ns->geom.pgshift;
        uint pg_off = off&(ns->geom.oobsz-1);

        return pg * (ns->geom.oobsz) + pg_off;
}

static int ns_erase(struct mtd_info *mtd, struct erase_info *instr)
{
        struct nandsim *ns = (struct nandsim *)mtd->priv;

        D("mtdns: erase (%d,%d)\n", (int)instr->addr, 
          (int)(instr->len));

	if (instr->addr + instr->len > mtd->size)
		return -EINVAL;

        // must be block-aligned
        if((instr->addr % ns->geom.secsz) || (instr->len % ns->geom.secsz)) {
                printk(KERN_ERR "mtdns: erase not block aligned %d, %d\n",
                       (int)instr->addr, (int)instr->len);
                return -EINVAL;
        }

        // erase data
	memset(ns->mem + instr->addr, 0xff, instr->len);
        // erase oob data
        memset(ns->oob + oob_off(instr->addr), 0xff,
               (instr->len/ns->geom.pgsz)*ns->geom.oobsz);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int ns_read(struct mtd_info *mtd, loff_t from, size_t len,
                        size_t *retlen, u_char *buf)
{
        struct nandsim *ns = (struct nandsim *)mtd->priv;

        D("mtdns: read from (%d,%d)\n", (int)from, (int)(len));

	if (from + len > mtd->size)
		return -EINVAL;

	memcpy(buf, ns->mem + from, len);

	*retlen = len;
	return 0;
}

extern int ekm_is_traced_dev(int dev);
static int ns_write(struct mtd_info *mtd, loff_t to, size_t len,
                         size_t *retlen, const u_char *buf)
{
        struct nandsim *ns = (struct nandsim *)mtd->priv;

        D("mtdns: write to (%d,%d)\n", (int)to, (int)(len));

	if (to + len > ns->geom.totsz)
		return -EINVAL;

#if 0
        if((to&(ns->geom.pgsz-1)) || (len&(ns->geom.pgsz-1)))
                printk("mtdns: write is not page aligned");
        else
                printk("mtdns: write is page aligned"); 
#endif

        if(ekm_is_traced_dev(ns->dev))
                EXPLODE_HOOK(log_buffer_record, EKM_ADD_BUFFER,
                             ns->dev, to, len, buf);

	memcpy(ns->mem + to, buf, len);

	*retlen = len;
	return 0;
}


static int ns_read_oob(struct mtd_info *mtd, loff_t from, size_t len,
                        size_t *retlen, u_char *buf)
{
        struct nandsim *ns = (struct nandsim *)mtd->priv;

        D("mtdns: read_oob from (0x%x,%d)\n", (int)from, (int)(len));

        if(from&(ns->geom.pgsz-ns->geom.oobsz)) {
                // XXX this oob interface is stupid
                printk(KERN_ERR "mtdns: oob offset invalid\n");
                return -EINVAL;
        }

        from = oob_off(from);
	if (from + len > ns->geom.totoobsz)
		return -EINVAL;

	memcpy(buf, ns->oob + from, len);

	*retlen = len;
	return 0;
}

static int ns_write_oob(struct mtd_info *mtd, loff_t to, size_t len,
                         size_t *retlen, const u_char *buf)
{
        struct nandsim *ns = (struct nandsim *)mtd->priv;

        D("mtdns: write_oob to (0x%x,%d)\n", (int)to, (int)(len));

        if(to&(ns->geom.pgsz-ns->geom.oobsz)) {
                // XXX this oob interface is stupid
                printk(KERN_ERR "mtdns: oob offset invalid\n");
                return -EINVAL;
        }

        to = oob_off(to);
	if (to + len > ns->geom.totoobsz)
		return -EINVAL;

#if 0
        if((to&(ns->geom.oobsz-1)) || (len&(ns->geom.oobsz-1)))
                printk("mtdns: oob write is not page aligned");
        else
                printk("mtdns: oob write is page aligned"); 
#endif

        if(ekm_is_traced_dev(ns->dev))
                EXPLODE_HOOK(log_buffer_record, EKM_ADD_BUFFER,
                             ns->dev, to, len, buf);

	memcpy(ns->oob + to, buf, len);
        
	*retlen = len;
	return 0;
}

static int ns_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
                         size_t * retlen, u_char * buf, 
                         u_char * oob_buf, struct nand_oobinfo *oobsel)
{
        D("mtdns: read_ecc from (%d,%d)\n", (int)from, (int)(len));
        /* TODO: check ecc */
        return ns_read(mtd, from, len, retlen, buf);
}


static int ns_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
                          size_t * retlen, const u_char * buf, 
                          u_char * eccbuf, struct nand_oobinfo *oobsel)
{
        D("mtdns: write_ecc to (%d,%d)\n", (int)to, (int)(len));
        /* TODO: write ecc */
        return ns_write(mtd, to, len, retlen, buf);
}

static int ns_isbad(struct mtd_info *mtd, loff_t ofs)
{
        /* TODO */
        return 0;
}

static int ns_markbad(struct mtd_info *mtd, loff_t ofs)
{
        /* TODO */
        return 0;
}

static void ns_init_geom(struct nandsim *ns)
{
        ns->geom.totsz = totsz_in_kb * 1024;
        ns->geom.secsz = secsz_in_kb * 1024;
        ns->geom.pgsz = pgsz;
        ns->geom.oobsz = oobsz;

        ns->geom.pgnum = ns->geom.totsz/pgsz;
        ns->geom.pgsec = ns->geom.secsz/pgsz;
        ns->geom.pgszoob = pgsz + oobsz;
        ns->geom.secszoob = ns->geom.pgszoob * ns->geom.pgsec;
        ns->geom.totoobsz = oobsz * ns->geom.pgnum;

        ns->geom.pgshift = ffs(pgsz) -1;
}

static void ns_init_mtd(struct nandsim *ns)
{
	/* Setup the MTD structure */
        struct mtd_info *mtd = &ns->mtd;

	mtd->priv = ns;
	mtd->name = "simple mtd simulator";
        mtd->type = MTD_NANDFLASH;
        mtd->flags = MTD_CAP_NANDFLASH | MTD_ECC;

	mtd->size = ns->geom.totsz;
	mtd->erasesize = ns->geom.secsz;

        mtd->oobsize = ns->geom.oobsz;
        mtd->oobblock = ns->geom.pgsz;
        switch(mtd->oobsize) {
        case 16:
                memcpy(&mtd->oobinfo, &nand_oob_16, sizeof mtd->oobinfo);
                break;
        default:
                printk("mtdns: oobsize %d not implemented yet\n", mtd->oobsize);
        }

        mtd->ecctype = MTD_ECC_SW;
        mtd->eccsize = 256; /* compute ecc every 256 bytes */

	mtd->erase = ns_erase;

	mtd->read = ns_read;
        mtd->read_oob = ns_read_oob;
        mtd->read_ecc = ns_read_ecc;
        mtd->readv = default_mtd_readv;

	mtd->write = ns_write;
        mtd->write_oob = ns_write_oob;
        mtd->write_ecc = ns_write_ecc;
        mtd->writev = default_mtd_writev;

        mtd->block_isbad = ns_isbad;
        mtd->block_markbad = ns_markbad;

	mtd->owner = THIS_MODULE;
}

static struct nandsim *ns_new(void)
{
        struct nandsim *ns;

        ns = kmalloc(sizeof *ns, GFP_KERNEL);
        if(!ns)
                return NULL;
        memset(ns, 0, sizeof *ns);

        ns_init_geom(ns);

        /* Allocate and initialize internal buffer */
        ns->mem = vmalloc(ns->geom.totsz);
        if(!ns->mem) {
                kfree(ns);
                return NULL;
        }
	memset(ns->mem, 0xff, ns->geom.totsz);

        ns->oob = vmalloc(ns->geom.totoobsz);
        if(!ns->oob) {
                vfree(ns->mem);
                kfree(ns);
                return NULL;
        }
	memset(ns->oob, 0xff, ns->geom.totoobsz);

        /* Fill and register the mtd_info structure */
        ns_init_mtd(ns);

#ifdef CONFIG_MTD_PARTITIONS
	/* Fill the partition_info structure */
	ns->part.name   = "simple mtd simulator partition";
	ns->part.offset = 0;
	ns->part.size   = ns->geom.totsz;
#endif

        ns->dev = (SMTD_MAGIC_MAJOR<<8) + 0;

	D("flash size: %u MB\n",          ns->geom.totsz >> 20);
	D("page size: %u bytes\n",         ns->geom.pgsz);
	D("page OOB area size: %u bytes\n",     ns->geom.oobsz);
	D("sector size: %u KiB\n",         ns->geom.secsz >> 10);
	D("pages number: %u\n",            ns->geom.pgnum);
	D("pages per sector: %u\n",        ns->geom.pgsec);
	D("total OOB area size: %u B\n", ns->geom.totoobsz);
	D("page shift: %u\n",              ns->geom.pgshift);

	return ns;
}

static void ns_del(struct nandsim *ns)
{
        vfree(ns->oob);
        vfree(ns->mem);
        kfree(ns);
}

static int __init init_mtdns(void)
{
	if (!totsz_in_kb)
		return -EINVAL;

        ns = ns_new();
        if(!ns)
                return -ENOMEM;

        /* register mtd device */
        if (add_mtd_device(&ns->mtd)) {
                ns_del(ns);
                return -EIO;
        }

        D("added mtd\n");
           
#ifdef CONFIG_MTD_PARTITIONS
        /* register as one big partition */
	if (add_mtd_partitions(&ns->mtd, &ns->part, 1)) {
                ns_del(ns);
		return -EIO;
	}
        D("added mtd partitions\n");
#endif


        return 0;
}

static void __exit cleanup_mtdns(void)
{
	if (!ns) 
                return;

#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions (&ns->mtd);
#endif
	/* Deregister the device */
        del_mtd_device(&ns->mtd);

        ns_del(ns);
        ns = 0;

        D("removed smtd\n");
}

module_init(init_mtdns);
module_exit(cleanup_mtdns);

MODULE_LICENSE("GPL");

/*
  MTD functions jffs2 used:

  block_isbad
  block_markbad

  eccsize
  ecctype

  erase
  erasesize

  index
  name
  size
  sync
  type

  oobblock
  oobinfo
  oobsize

  read
  read_ecc
  read_oob
  write
  write_ecc
  writev
  writev_ecc
*/
