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


#ifndef __WRITES_H
#define __WRITES_H

#define BIG_FILE_LEN (1ULL<<30)
#define MEDIUM_FILE_LEN (1ULL<<15)
#define MAX_SIG_WRITE (20)
#define NUM_WRITES (5)

#include <vector>

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mcl.h"

//#define CHECK_RETURN(x) explode_assert(x >= 0, "system call failed while marshalling the actual file system: %s, %s", path, strerror(errno))
#ifndef DEBUG
#	define CHECK_RETURN(x) do {if(x < 0) {perror(NULL); error("inconsistent", "system call failed while marshalling the actual file system: %s, %s", path, strerror(errno));}} while(0)
#else
#	define CHECK_RETURN(x) if(x < 0) abort()
#endif

typedef struct write_s {
	off_t off;
	size_t len;
	char buf[MAX_SIG_WRITE * sizeof(signature_t)];
	off_t size; // file size;
#if 0
	signature_t sig; // file sig;
#endif
} write_t;

void init_writes (void);
signature_t hash_file(const char* path, struct stat &st);

const int position_magic[] = {1, 2};//values can't be zero
const int nposition_magic = 
	sizeof position_magic / sizeof position_magic[0];
signature_t position_hash(off_t o, int magic);
unsigned next_write_by_size(off_t size);

extern int nwrites;
extern write_t writes[][nposition_magic];
#endif
