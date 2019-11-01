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


#ifndef __ARCHIVE_H
#define __ARCHIVE_H

/* Now an archive is just a simple self-expandable memory region.
   now the region gets freed in destructor. should possibly use
   ref counting.  use this for all the checkpoints, disks etc.  We
   can do possible copy-on-write here */

#include <utility>
#include <string>

#include <assert.h>

#include "mcl_types.h"

#define ARCHIVE_MIN_LEN (16)
class Archive {
 private:
	char *buf;
	char *cur_wr;
	char *cur_rd;
	int len;

	void check_expand(unsigned l);
	void init(unsigned l);
 public:
	Archive(void) {init(ARCHIVE_MIN_LEN);}
	Archive(unsigned l) {init(l);}
	Archive(char* b, unsigned l) {
		buf = cur_rd = b; 
		len = l; cur_wr = buf + l;
	}

	~Archive(void) { free(buf);}
	
	void put(const char* p, unsigned l);
	void get(char *p, unsigned l);
	void put(std::string& s);
	void get(std::string& s);

	// FIXME: this is dangerous
	template <class T>
	Archive& operator<<(T v) {
		put((const char*)&v, sizeof(v));
		return *this;
	}

	// FIXME: this is dangerous
	template<class T>
	Archive& operator>>(T &v) {
		get((char*)&v, sizeof(v));
		return *this;
	}

	bool eof(void) {
		return cur_rd >= cur_wr;
	}

	void clear(void) {cur_rd = cur_wr = buf;}
	
	signature_t get_sig(void);
	
	// pass buf to us. now we have the ownership of this buf
	void attach(char *buf, int max);
	// after return, we no longer have ownership of buf
	char* detach(int &max);
	unsigned read_pos(void) {return cur_rd - buf;}
	unsigned write_pos(void) {return cur_wr - buf;}

	unsigned size(void) {return cur_wr - buf;}
	unsigned max_size(void) { return len;}
	void resize(int len);
	const char* buffer(void) {return buf;}
};

#endif
