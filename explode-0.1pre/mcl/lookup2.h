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


#ifndef __lookup_2_h
#define __lookup_2_h

typedef  unsigned int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned short int  ub2;
typedef  unsigned       char ub1;

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

#ifdef __cplusplus
extern "C" {
#endif

ub4 hash(ub1 *k, ub4 length, ub4 initval);
ub4 hash2(ub4 *k, ub4 length, ub4 initval);
ub4 hash3(ub1 *k, ub4 length, ub4 initval);

#ifdef __cplusplus
}
#endif

#endif
