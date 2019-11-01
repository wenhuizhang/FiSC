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


#ifndef __RDD_H
#define __RDD_H

#include <linux/ioctl.h>

/* this shouldn't conflict with anything listed in
   Documentation/ioctl-number.txt */
#define RDD_IOC_MAGIC (0xD0)

#define RDD_GET_DISK _IOR(RDD_IOC_MAGIC, 1, char *)
#define RDD_SET_DISK _IOR(RDD_IOC_MAGIC, 2, char *)
#define RDD_COPY_DISK _IOR(RDD_IOC_MAGIC, 3, int)

#endif
