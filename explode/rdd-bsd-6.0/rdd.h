#ifndef __RDD_H
#define __RDD_H

#if ! defined _KERNEL
#include <sys/ioctl.h>
#endif

static int hardsect_size = 512;  /* Hardsector Size */
static int nsectors = 1024;  /* How big the drive is */
static int ndisks = 3;  /* Number of Disks */
//static int npartitions = 16;  /* Number of partitions per disk */
//static int rdd_debug = 0;

/* this shouldn't conflict with anything listed in
   Documentation/ioctl-number.txt */
#define RDD_IOC_MAGIC (0xD0)

#define RDD_GET_DISK _IO(RDD_IOC_MAGIC, 1)
#define RDD_SET_DISK _IO(RDD_IOC_MAGIC, 2)
#define RDD_COPY_DISK _IO(RDD_IOC_MAGIC, 3)

#endif
