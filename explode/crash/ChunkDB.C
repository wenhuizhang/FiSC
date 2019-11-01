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


#include "mcl.h"
#include "ChunkDB.h"
#include "BlockDev.h"

void ChunkDB::init(unsigned usz)
{
	unit_size = usz;
	while(usz>1) {
		unit_bits ++;
		usz >>= 1;
	}
	
	char *buf = new char[unit_size];
	assert(buf);
	
	// store a garbage sector into the db.  sometimes they do
	// read uninitialized sector.
	int w = GARBAGE_WORD;
	char *p = buf;
	while(p < buf + unit_size) {
		memcpy(p, &w, sizeof w);
		p += sizeof w;
	}
	//memset(buf, 0xFF, unit_size);

	_invalid_sec = write((const char*)buf);

	delete[]buf;
}

void ChunkDB::cleanup(void)
{
	chunk_db_t::iterator it;
	for_all_iter(it, db)
		delete [] (*it).second;
}

#include <iostream>
using namespace std;
#define CHECK_COLLISION
signature_t ChunkDB::write(const char* b)
{
	signature_t sig = explode_hash((ub1*)b, unit_size);

#ifdef CHECK_COLLISION
	if(db.find(sig) != db.end()) {
		char* unit = db[sig];
		if(memcmp(unit, b, unit_size))
			assert(0);
	}
#endif
	if (db.find(sig) != db.end())
		return sig;

	// TODO: filter out time vars
	char *unit = new char[unit_size];
	assert(unit);
	memcpy (unit, b, unit_size);
	db[sig] = unit;
	if(db.size()%1000 == 0)
		v_out << "ChunkDB mem ="
			  << (db.size()/(1024/SECTOR_SIZE))
			  << "KB\n";
	return sig;
}

void ChunkDB::read(signature_t sig, char* b)
{
	chunk_db_t::iterator it = db.find(sig);
#if 1
	if(it == db.end())
		cerr << "ChunkDB::read unknown sig=" << sig << endl;
#endif
	assert(it != db.end());
	memcpy(b, (*it).second, unit_size);
}

const char* ChunkDB::read(signature_t sig)
{
	chunk_db_t::iterator it = db.find(sig);
#if 1
	if(it == db.end())
		cerr << "ChunkDB::read unknown sig=" << sig << endl;
#endif
	assert(it != db.end());
	return (*it).second;
}

ChunkDB db;
