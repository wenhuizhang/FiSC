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


#ifndef __DATA_H
#define __DATA_H

#include <map>
#include <limits.h>

#include "mcl_types.h"

struct data_region {
	off_t start, end;

	int magic; // 0 means not a magic data region.
		   // > 0 means region whose content equals
		   // position_hash(offset of the buffer, magic)
	char* data;

	data_region(void);
	data_region(off_t o, off_t o_e, int m, const char* d);
	void init(off_t o, off_t o_e, int m, 
		  char* d, bool copy_p);
	~data_region(void);
	void clear(void);
	void copy(struct data_region &wr);
	struct data_region *clone(void);
	void internal_check(void);
};

class region_visitor {
 public:
	virtual void operator()(struct data_region *r) {}
	virtual ~region_visitor() {}
};

// now support read,write,truncate with arbitrary offset and size
class data: public std::map<off_t, struct data_region*> {
 private:
	typedef std::map<off_t, struct data_region*> parent_t;

	enum region_type {ZERO, DATA};
	char* _buf;
	off_t _buf_start;
	off_t _buf_end;
	enum region_type get_type(signature_t sig, off_t off, int& magic);
	bool next_region(off_t start, off_t& end, 
			 enum region_type& type, int& magic);
	void write_region(off_t start, off_t end, 
			  enum region_type type, int magic, 
			  const void* buf);
	void zero_range(off_t start, off_t end);

 public:
	~data() {clear();}
	// first <= it < second
	typedef std::pair<iterator, iterator> iterator_range_t;
	void internal_check(void);
	iterator_range_t intersected_range(off_t start_in, off_t end_in,
					   off_t& start, off_t& end);
	iterator_range_t intersected_range(off_t start_in, off_t end_in);
	void clear(void);
	void copy(data &dt);
	// this passes the ownership of dr from the caller to
	// this data object
	void add(struct data_region *dr);

	void pwrite(const void *buf, size_t size, 
		    off_t off);
	void pread(void *buf, size_t size, off_t off);
	void truncate(off_t length);
	void print(std::ostream& out) const;

	void for_each_region(region_visitor* v);
};

std::ostream& operator<<(std::ostream& o, const data& d);

#endif
