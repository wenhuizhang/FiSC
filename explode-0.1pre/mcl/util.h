/* -*- Mode: C++ -*- */

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


#ifndef __EXPLODE_UTIL_H
#define __EXPLODE_UTIL_H

/* constants */
#define SECTOR_SIZE (512U)
#define SECTOR_SIZE_BITS (9)

#define MAX_NAME (256)

#ifdef __linux__
#  define MAX_PATH (4096)
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#  define MAX_PATH (1023)
#else
#  error "os not supported. don't know how to load kernel module!"
#endif

#include "mcl-options.h"
#include "output.h"

// define this so we can easily switch to static_cast if need be
#define CAST(T, x) dynamic_cast<T>(x)

#define ROUND_DOWN(x, align) ((x)&~((align)-1))
#define ROUND_UP(x, align)  ROUND_DOWN(((x)+(align)-1), align)

#ifdef __cplusplus
extern "C" {
#endif
	
    int execv_interposed(const char* path, int verbose, char *const argv[]);
    extern int track_cmd;
    int systemf(const char *fmt, ...);

    const char* to_hex(char c);
    const char* buf_to_hex(const char* buf, int len);
    int memcmp_long(const char* d1, const char* d2, size_t size);
	
    // return values need to be deleted using delete []
    char* xstrdup(const char* s);
    char* xbufdup(const char* buf, int len);
	
    void xwrite(int fd, const void* buf, size_t count);

    ssize_t xrecv (int fd, void* b, size_t count, int flags);
    ssize_t xsend (int fd, const void* b, size_t count, int flags);

    int xinet_aton(const char* hostname, struct in_addr *inp);
	
    /* Obtain a backtrace and print it to stdout. */
    void print_trace(void);

#ifdef __cplusplus
}
#endif

#define abort_on(s)  \
	do { \
		perror(#s); \
		abort(); \
	} while(0)\

#ifdef __cplusplus
// useful macroes
#    define for_all(i, v) \
	    for((i) = 0; (i) < (int)(v).size(); ++(i))
#    define for_all_iter(it, v) \
	    for((it) = (v).begin(); (it) != (v).end(); ++(it))
#    define forit(it, v) \
	    for((it) = (v).begin(); (it) != (v).end(); ++(it))

template<class I, class C>
static inline bool exists(I i, C c) {
    return c.find(i) != c.end();
}

#endif

#endif
