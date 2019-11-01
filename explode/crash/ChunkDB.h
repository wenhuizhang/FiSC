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


#ifndef __CHUNKDB_H
#define __CHUNKDB_H

#include "hash_map_compat.h"
#include "lookup2.h"
#include "mcl_types.h"

#define INVALID_SECTOR ((ub4)-1)
#define MAX_UNIT_SIZE (1U<<20)

class ChunkDB
{
 public:
	// let's not worry about collions
	typedef Sgi::hash_map<signature_t, char*> chunk_db_t;
	chunk_db_t db;

	unsigned unit_size;
	int unit_bits;
	signature_t _invalid_sec;

	unsigned _time_mask;
	unsigned _default_time;

	ChunkDB(void) { init(SECTOR_SIZE); }
	ChunkDB(unsigned unit_size) {init(unit_size);}
	~ChunkDB(void) {cleanup();}
	
	signature_t write(const char* b);
	void read (signature_t, char* b);
	const char* read(signature_t);
	void init(unsigned unit_size);
	void cleanup(void);

	bool is_valid(signature_t s) {
		return s != _invalid_sec;
	}
};

extern ChunkDB db;
#endif
