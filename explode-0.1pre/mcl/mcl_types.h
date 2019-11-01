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


#ifndef __EXPLODE_TYPES_H
#define __EXPLODE_TYPES_H

/* types */

// it's so nice that hashes returned by lookup2 can fit into
// 4-bypte.  Another nice feature is that lookup2 doesn't do mod,
// which means it is fast.  Bad thing is that the collision rate
// is like 1 out of 2^32.  let's try lookup2 hash first.  If
// collision rate becomes a problem, we can always switch to md4
// or md5

#include "lookup2.h"
typedef unsigned signature_t;
#define explode_hash(buf, len) hash3(buf, len, 0)

typedef unsigned char choice_t;
#define MAX_CEIL ((choice_t)(-1))

#ifdef __cplusplus
#include <cstring>
#include <vector>
typedef std::vector<choice_t> trace_t;

struct ltstr {
	bool operator()(const char* s1, const char* s2) const {
		return strcmp(s1, s2) < 0;
	}
};

struct eqstr {
	bool operator()(const char* s1, const char* s2) const {
		return strcmp(s1, s2) == 0;
	}
};
#endif

#ifdef __i386__
typedef unsigned long long uint64_t;
typedef long long int64_t;
#elif defined(__x86_64__)
#endif

#endif
