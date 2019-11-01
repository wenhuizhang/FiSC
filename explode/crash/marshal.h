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


#ifndef __MARSHAL_H
#define __MARSHAL_H

#ifdef __KERNEL__
#define DIE(s) panic(s)
#define ALLOC(size) kmalloc(size, GFP_ATOMIC)
#else
#include "mcl.h"
#define DIE(s) xi_panic(s)
#define ALLOC(size) new char[size]
#endif

/* should just use C++ to write the kernel modules, so all these
   nasty macros become template functions */
#define DEF_MARSHAL_SCALAR(type, name) \
static inline int name##_marshal_len(type value) \
{ \
	return sizeof value; \
} \
static inline void marshal_##name(char* data, int len,  \
			int *off, type value) \
{ \
	if((int)(*off + sizeof value) > len) \
		DIE("marshall overflow!"); \
	memcpy(data+*off, (const char*)&value, sizeof value); \
	*off += sizeof value; \
} \
\
static inline type unmarshal_##name(const char* data, int len, int *off) \
{ \
	type value; \
	if((int)(*off + sizeof value) > len) \
		DIE("unmarshall overflow!"); \
	memcpy((char*)&value, data+*off, sizeof value); \
	*off += sizeof value; \
	return value; \
}

DEF_MARSHAL_SCALAR(char, char)
DEF_MARSHAL_SCALAR(short, short)
DEF_MARSHAL_SCALAR(int, int);
DEF_MARSHAL_SCALAR(long, long);
#ifndef __KERNEL__
DEF_MARSHAL_SCALAR(mode_t, mode);
DEF_MARSHAL_SCALAR(off_t, off);
DEF_MARSHAL_SCALAR(signature_t, signature);
#endif
/*DEF_MARSHAL_SCALAR(unsigned int, unsigned_int);
  DEF_MARSHAL_SCALAR(unsigned long, unsigned_long);*/

static inline int str_marshal_len (const char* str)
{
	return sizeof(int) /* length */ + strlen(str);
}

static inline void marshal_str(char *data, int len, int *off, const char* str)
{
	int l = strlen(str);
	if(str_marshal_len(str) + *off > len)
		DIE("marshal_str overflow!");
	marshal_int(data, len, off, l);
	strncpy(data+*off, str, l);
	*off += l;
}

static inline char* unmarshal_str(const char* data, int len, int *off)
{
	char *s;

	int l = unmarshal_int(data, len, off);
	if(l + *off > len)
		DIE("unmarshal_str overflow!");
	
	s = (char*)ALLOC(l + 1);
	if(!s)
		DIE("unmarshal_str out of mem!");
	strncpy(s, data+*off, l);
	s[l] = 0;
	*off += l;
	return s;
}

static inline char* unmarshal_buf(const char* data, int len, int *off, 
				  int buf_len)
{
	char *s;
	if(buf_len + *off > len)
		DIE("unmarshal_str overflow!");
	s = (char*)ALLOC(buf_len);
	if(!s)
		DIE("unmarshal_str out of mem!");
	memcpy(s, data+*off, buf_len);
	*off += buf_len;
	return s;
}

static inline void unmarshal_buf_to(const char* data, int len, int *off, 
				    int buf_len, void* buf)
{
	if(buf_len + *off > len)
		DIE("unmarshal_str overflow!");
	if(!buf)
		DIE("unmarshal_str out of mem!");
	memcpy(buf, data+*off, buf_len);
	*off += buf_len;
}

#endif
