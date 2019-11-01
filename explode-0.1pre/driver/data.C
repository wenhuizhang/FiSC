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


#include <sys/types.h>

#include <list>
#include <iostream>

#include "mcl_types.h"
#include "writes.h"
#include "marshal.h"
#include "data.h"

const off_t EXPLODE_OFF_MAX = (INT_MAX & ~(sizeof(signature_t)-1));
//#define DEBUG
#ifndef DEBUG
#	include "util.h"
#else
char *xbufdup(const char* buf, int len)
{
	char *p = new char[len];
	assert(p);
	memcpy(p, buf, len);
	return p;
}

#include <stdarg.h>
void explode_assert_failed(const char* file, 
		  int line, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "Assertion failed at %s, %d: ", file, line);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "!\n");
	va_end(ap);
	abort();
}
#endif

#include "data.h"

#define ALIGN (sizeof(signature_t))

using namespace std;

data_region::data_region(void)
{
	start = 0;
	end = 0;
	magic = 0;
	data = NULL;
}

void data_region::init(off_t o, off_t o_e, int m, 
					   char* d, bool copy_p)
{
	start = o;
	end = o_e;
	magic = m;
	if(m)
		data = NULL;
	else if(copy_p)
		data = xbufdup(d, o_e - o);
	else
		data = d;
}

data_region::data_region(off_t o, off_t o_e, int m, const char* d)
{
	init(o, o_e, m, (char*)d, true);
}

void data_region::clear(void)
{
	internal_check();
	if(data) delete [] data;
	data = NULL;
	magic = 0;
	start = 0;
	end = 0;
}

data_region::~data_region(void)
{
	clear();
}

signature_t get_sig(const char* data)
{
	int o = 0;
	return unmarshal_signature(data, ALIGN, &o);
}

void data_region::internal_check(void)
{
	assert(end <= EXPLODE_OFF_MAX);
	if(magic) {
		assert(!data);
		assert(end > start);
	}
	else if (data){
		assert(end > start);
		signature_t first, last;
		first = get_sig(data);
		last = get_sig(data+end-start-ALIGN);
		
		assert(first != 0 && last != 0);
	}
	else if(!data) // empty data region
		assert(end == start);
}

struct data_region *data_region::clone(void)
{
	struct data_region *dr = new struct data_region;
	assert(dr);
	dr->copy(*this);
	return dr;
}

void data_region::copy(struct data_region &wr)
{
	start = wr.start;
	end = wr.end;

	if(data) {
		assert(!magic && end > start);
		delete [] data;
	}
	data = NULL;

	magic = wr.magic;
	if(wr.magic == 0) {
		assert(wr.data);
		data = xbufdup(wr.data, wr.end - wr.start);
	}
}

void data::internal_check(void)
{
	iterator it;
	off_t start;
	struct data_region *dr, *prev_dr=NULL;
	for(it = begin(); it != end(); it++) {
		start = (*it).first;
		dr = (*it).second;

		// now we require the data regions we have are aligned
		assert((dr->start%ALIGN) == 0);
		assert((dr->end%ALIGN) == 0);

		// offset isn't changed outside us
		assert(start == dr->start);

		// don't overlap
		if(prev_dr) {
			if(prev_dr->magic == dr->magic) 
				// they shoudl be merged if their magic values equal
				assert(prev_dr->end < dr->start);
			else
				assert(prev_dr->end <= dr->start);
		}

		// same magic
		if(prev_dr)
			assert(prev_dr->magic == dr->magic);

		// data region is internally consistent		
		dr->internal_check();
		prev_dr = dr;
	}
}

// regions inside [return.first, return.second) should all intersect with r
data::iterator_range_t data::intersected_range(off_t start_in, off_t end_in,
									  off_t& start, off_t& end)
{
	iterator low_it, high_it, it;
	map<off_t, bool> intersect_list;
	map<off_t, bool>::iterator dit;
	struct data_region *r_cur;

	// set default result here
	start = start_in;
	end = end_in;

	it = lower_bound(start_in);//min start, s.t. start >= start_in
	if(it == begin())
		;//start = start_in; // no such start exists
	else {
		it --; // max start, s.t. start < start_in
		r_cur = (*it).second;
		if(r_cur->end >= start_in) {
			// update result
			start = r_cur->start;
			// intersect
			intersect_list[r_cur->start] = true;
		}
		else {
			//start = start_in;
			it ++;
		}
	}

	// now r_cur->start >= start_in
	while(it != this->end()) {
		r_cur = (*it).second;

		// start_in end_in start end
		if(r_cur->start > end_in) {
			// update result
			// end = end_in;
			// done
			break;
		}
		
		// start_in start end_in end
		if(r_cur->end > end_in) {
			// update result
			end = r_cur->end;
			// intersect
			intersect_list[r_cur->start] = true;
			// done
			break;
		}

		// start_in start end end_in
		// intersect
		intersect_list[r_cur->start] = true;
		// not done yet. keep going
		it ++;
	}

	low_it = high_it = this->end();
	if(intersect_list.size() > 0) {
		dit = intersect_list.begin();
		low_it = find((*dit).first);
		assert(low_it != this->end());
		dit = intersect_list.end();
		dit--;
		high_it = find((*dit).first);
		assert(high_it != this->end());
		high_it ++;
	}

	return iterator_range_t(low_it, high_it);
}

data::iterator_range_t data::intersected_range(off_t start_in, off_t end_in){
	off_t s, e;
	return intersected_range(start_in, end_in, s, e);
}

void data::copy(data& dt)
{
	clear();

	dt.internal_check();
	iterator it;
	for(it = dt.begin(); it != dt.end(); it++) {
		struct data_region* dr = (*it).second;
		(*this)[(*it).first] = dr->clone();
	}
}

void data::clear(void)
{
	iterator it;
	for(it = begin(); it != end(); it++)
		delete (*it).second;
	parent_t::clear();
}

void data::add(struct data_region *dr)
{
	(*this)[dr->start] = dr;
}

#if 0
// trim leading and tailing zeroes of a buffer.  keep the results word aligned
static void trim_buf(char*& buf, off_t &start, off_t &end)
{
	int size = end - start;
	int i = 0, j = size-1;

	while(i < (int)size && !buf[i]) i++;
	while(j > i && !buf[j]) j--;
	j++;
	// round down
	i = ROUND_DOWN(i, ALIGN);
	// round up
	j = ROUND_UP(j, ALIGN);
	buf = buf + i;
	end = start+j;
	start = start + i;
}
#endif

// this is the most complicated function of this class
// write buf to [start, end)
// type == ZERO ==> zero range
// type == DATA && magic == 0 => regular write
// type == DATA && magic != 0 => magic write
void data::write_region(off_t start, off_t end, 
						enum region_type type,
						int magic, const void* buf)
{
	iterator_range_t range;
	iterator it, next;
	struct data_region *dr;
	struct data_region *dr_insert_front, *dr_insert_back;
	struct data_region *dr_merge_front, *dr_merge_back;
	struct data_region *dr_write;

	range = intersected_range(start, end);
	dr_insert_front = dr_insert_back = NULL;
	dr_merge_front = dr_merge_back = NULL;
	next = range.first;
	while(next != range.second) {
		it = next++;
		dr = (*it).second;

		// newstart oldstart oldend newend
		if(dr->start >= start && dr->end <= end) {
			// dr is completely overwritten. remove
			erase(it); // good thing about map: erase only
					   // invalidates the iterator we erase
			delete dr;
			continue;
		}

		// oldstart newstart oldend newend
		if(dr->start < start && dr->end <= end) {
			assert(start <= dr->end);
			if(type == ZERO || dr->magic != magic) {
				if(start == dr->end)
					continue;
				// overlap, simply truncate since we don't need to
				// update the key dr->off
				dr->end = start;
				if(!dr->magic)
					memset(dr->data + start - dr->start, 0,
						   dr->end - start);
				continue;
			}
			// write of same kind
			// record that we should merge this region
			assert(dr_merge_front == NULL);
			erase(it);
			dr_merge_front = dr;
			continue;
		}

		// oldstart newstart newend oldend
		if(dr->start < start && dr->end > end) {
			if(type == ZERO || dr->magic != magic) {
				// break dr into two data regions
				assert(dr_insert_front == NULL);
				dr_insert_front = new struct data_region(dr->start, start, 
														 dr->magic, dr->data);
				assert(dr_insert_front);
				assert(dr_insert_back == NULL);
				dr_insert_back = new struct data_region(end, dr->end, 
														dr->magic, 
														dr->data+end);
				assert(dr_insert_back);
				erase(it);
				delete dr;
				continue;
			}

			// write of same kind.  update in place. do nothing if
			// magic != 0
			if(!magic)
				memcpy(dr->data + start - dr->start,
					   buf,
					   end - start);
			// we are all done at this point. shouldn't have any
			// pending insertions or merges
			assert(dr_insert_front == NULL 
				   && dr_insert_back == NULL
				   && dr_merge_front == NULL
				   && dr_merge_back == NULL);
			return;
		}

		// newstart oldstart newend oldend
		assert(dr->start >= start && dr->end > end);
		assert(end >= dr->start);
		if(type == ZERO || dr->magic != magic) {
			if(end == dr->start) 
				continue;
			// trim dr's head
			// have to insert. no easy way to update (*it).first
			assert(dr_insert_back == NULL);
			dr_insert_back = new struct data_region(end, dr->end, 
													dr->magic,
													dr->data+end-dr->start);
			assert(dr_insert_back);
			erase(it);
			delete dr;
			continue;
		}
		
		// write of same kind
		// record that we should merge this region
		assert(dr_merge_back == NULL);
		erase(it);
		dr_merge_back = dr;
	}

	// insert
	if(dr_insert_front)
		add(dr_insert_front);
	if(dr_insert_back)
		add(dr_insert_back);

	if(type == ZERO)
		return;  // all done at this moment

	// handle merge & write
	dr_write = new struct data_region;
	assert(dr_write);
	dr_write->magic = magic;
	dr_write->start = start;
	if(dr_merge_front)
		dr_write->start = dr_merge_front->start;
	dr_write->end = end;
	if(dr_merge_back)
		dr_write->end = dr_merge_back->end;
	if(!magic) {
		dr_write->data = new char[dr_write->end - dr_write->start];
		assert(dr_write->data);
		// copy old data
		if(dr_merge_front)
			memcpy(dr_write->data,
				   dr_merge_front->data, 
				   start-dr_merge_front->start);
		if(dr_merge_back)
			memcpy(dr_write->data+dr_merge_back->start-dr_write->start,
				   dr_merge_back->data,
				   dr_merge_back->end-end);
		
		memcpy(dr_write->data+start-dr_write->start,
			   buf,
			   end-start);
	}
	add(dr_write);
}

// TODO: write a small program to find out what off and magic make
// position_hash(off, magic) return zero
static void lucky(off_t off) {
	cout <<"position hash of " << off
		 << "is zero! you should go buy lottery!";
	assert(0);
}

enum data::region_type 
data::get_type(signature_t sig, off_t off, int& magic)
{
	if(sig == 0)
		return ZERO; // region of zero
	
	int j;
	magic = 0;
	for(j = 0; j < nposition_magic; j++)
		if(sig == position_hash(off,
								position_magic[j])) {
			magic = position_magic[j];
			break;
		}

	return DATA; // region of data, either regular or magic (in
				 // which case, magic is set properly)
}

// returns a pure region that is either all zeroes, regular data,
// or magic data with the same magic number
bool data::next_region(off_t start, off_t& end, 
					   enum region_type& type, int& magic)
{

	signature_t sig;
	const char* p;
	enum region_type new_type;
	int new_magic;

	assert(start <= _buf_end);
	if(start ==_buf_end)
		return false;

	p = _buf + start - _buf_start;
	end = start;
	magic = 0;

	while(end <_buf_end) {
		sig = get_sig(p);
		if(end == start) // first sig fixes type and magic
			type = get_type(sig, end, magic);
		else {
			new_type = get_type(sig, end, new_magic);
			if(new_type != type
			   || (type == DATA && magic != new_magic))
				break;
		}

		p += ALIGN;
		end += ALIGN;
	}
	assert(end <=_buf_end);
	return true;
}

void data::pwrite(const void* b, size_t size, off_t off)
{

	// align write
	if(off%ALIGN || (off+size)%ALIGN) {
		off_t old_start = off;
		off_t old_end = off+size;

		_buf_start = ROUND_DOWN(old_start, ALIGN);
		_buf_end = ROUND_UP(old_end, ALIGN);

		assert(!_buf);
		_buf = new char[_buf_end-_buf_start];
		assert(_buf);
		if(old_start > _buf_start)
			pread(_buf, old_start-_buf_start, _buf_start);
		if(_buf_end > old_end)
			pread(_buf+old_end-_buf_start, _buf_end-old_end, old_end);
		memcpy(_buf+old_start-_buf_start, b, size);
	} else {
		_buf_start = off;
		_buf_end = off+size;
		_buf = (char*)b;
	}

	off_t start, end;
	enum region_type type;
	int magic;

	// write each region
	start = _buf_start;
	while(next_region(start, end, type, magic)) {
		write_region(start, end,
					 type, magic, _buf+start-_buf_start);
		start = end;
	}

	if(_buf != b)
		delete [] _buf;
	_buf = NULL;
}

void data::pread(void* b, size_t size, off_t off)
{
	off_t start, end, delta;
	struct data_region *dr;
	iterator it;
	char *buf;

	// easy but slow implementation
	iterator_range_t range = intersected_range(off, off+size, start, end);
	buf = new char[end-start];
	assert(buf);
	memset(buf, 0, size);
	for(it = range.first; it != range.second; it++) {
		dr = (*it).second;
		assert(dr->start >= start && dr->end <= end);
		delta = dr->start - start;

		if(!dr->magic) {
			// regular data
			memcpy(buf+delta, dr->data, dr->end - dr->start);
			continue;
		}

		// magic data
		assert(dr->start %ALIGN == 0
			   && dr->end %ALIGN == 0);
		off_t off_tmp = dr->start;
		char* buf_tmp = buf + delta;
		int cnt = (dr->end-dr->start)/ALIGN;
		int i;
		for(i = 0; i < cnt; i ++) {
			signature_t sig = position_hash(off_tmp, dr->magic);
			memcpy(buf_tmp, (const char*)&sig, sizeof sig);
			if(sig == 0) 
				lucky(off_tmp);
			buf_tmp = (char*)buf_tmp + sizeof sig;
			off_tmp += sizeof sig;
		}
	}

	assert(start <= off && (int)(off+size) <= end);
	memcpy(b, buf+off-start, size);
	delete [] buf;
}

void data::zero_range(off_t start, off_t end)
{
	write_region(start, end, ZERO, 0, NULL);
}

void data::truncate(off_t length)
{
	iterator it, next;
	iterator_range_t range;
	off_t length_aligned;

	length_aligned = ROUND_UP(length, ALIGN);
	if(length < length_aligned) {
		char zeroes[] = {0, 0, 0, 0};
		pwrite(zeroes, length_aligned-length, length);
	}
	zero_range(length_aligned, EXPLODE_OFF_MAX);
}

void data::print(ostream& out) const
{
	const_iterator it;

	if(size() == 0)
		cout << "empty" << endl;;
	
	for(it = begin(); it != end(); it++) {
		struct data_region *dr = (*it).second;
		cout << "[" << dr->start << ", " 
			 << dr->end;
		if(dr->magic == 0) {
			cout<<", "<<(void*)dr->data<<", ";
			cout<<buf_to_hex(dr->data, dr->end-dr->start);
		} else
			cout <<", "<< "magic=" << dr->magic;
		cout << ")";
	}
}

ostream& operator<<(ostream& o, const data& d)
{
	d.print(o);
	return o;
}

void data::for_each_region(region_visitor* v)
{
	iterator it;
	for(it = begin(); it != end(); it++)
		(*v)((*it).second);
}

#ifdef DEBUG

// command line to compile the debug version of this file:  
// g++ -DDEBUG -g -I../include data.C ../lib/lookup2.o ../lib/malloc.o writes.C -ldmalloc

#include "writes.h"

struct data_region r;
struct data_region drs[10];
int num = (sizeof drs)/(sizeof drs[0]);
data d;
data::iterator low, high;
data::iterator_range_t p;

void one_test()
{
	off_t off, end;
	p = d.intersected_range(r.start, r.end, off, end);
	low = p.first;
	high = p.second;

	cout << "final interval (" 
		 << off << ", "
		 << end << ")\n";
	cout << "all intersected intervals:\n";
	for(; low != high; low++)
		cout << "[" << (*low).first << ", " 
			 << (*low).second->end << ")" << endl;

	cout << "all intervals:\n";
	d.print(cout);
}

int main(void)
{
	r.magic = 1;
	r.start = 0;
	r.end = 4;
	
	cout<<"insert to an empty interval set. should return [0,4) and {}\n";
	one_test();
	
	int i;
	for(i = 0; i < num; i++) {
		drs[i].magic = 1;
		drs[i].start = (i+1) * 100;
		drs[i].end = drs[i].start + 40;
		cout << "region " << i << " ["
			 << drs[i].start << ", " << drs[i].end << ")\n";
		d[drs[i].start] = drs[i].clone();
	}

	cout<<"insert to beginning, no overlap.  should return [0,4) and {}\n";
	one_test();

	cout<<"insert to beginning, overlap. should return [80, 140) and {[100,140)}\n";
	r.start = 80;
	r.end = 128;
	one_test();

	cout<<"insert to beginning, contain. should return [80,100) and {[100,140)}\n";
	r.start = 80;
	r.end = 100;
	one_test();

	cout<<"insert to beginning, overlap with second interval. should return [80, 240) and {[100,140), [200,240)}\n";
	r.start = 80;
	r.end = 200;
	one_test();

	cout << "insert to end, no overlap. should return [1044, 2000) and {}\n";
	r.start = 1044;
	r.end = 2000;
	one_test();

	cout << "insert to end, overlap. should return [1000, 2000) and {[1000,1040)}\n";
	r.start = 1040;
	r.end = 2000;
	one_test();

	cout<<"insert, no overlap. should return [344, 396) and {}\n";
	r.start = 344;
	r.end = 396;
	one_test();

	cout<<"insert, overlap. should return [300, 440) and {[300,340), [400,440}\n";
	r.start = 340;
	r.end = 400;
	one_test();

	cout<<"insert, overlap many regions. should return [200, 940) and {[200,240), [300,340}, [400,440}, [400,540}, [600,640}, [700,740}, [800,840}, [900,940}\n";
	r.start = 240;
	r.end = 900;
	one_test();


	r.start = 1020;
	r.end = 1028;
	one_test();


	d.clear();
	char buf[1024];
	char zero_buf[1024] = {0};

	init_writes();
	cout<<"test pwrite then pread\n";
	for(i = 0; i < nwrites; i++) {
		d.pwrite(writes[i][0].buf, writes[i][0].len, writes[i][0].off);
		d.internal_check();
	}
	for(i = 0; i < nwrites; i++) {
		d.pread(buf, writes[i][0].len, writes[i][0].off);
		assert(memcmp(buf, writes[i][0].buf, writes[i][0].len) == 0);
	}

	cout<<"test truncate\n";
	for(i = nwrites-1; i > 1; i--) {
		d.truncate(writes[i-1][0].size-4);
		cout <<"after tuncate to " << writes[i-1][0].size-4 << ":\n";
		d.print(cout);
		d.pread(buf, writes[i][0].len+12, writes[i][0].off-12);
		assert(memcmp(buf, zero_buf, writes[i][0].len+12) == 0);
		d.pread(buf, writes[i-1][0].len, writes[i-1][0].off-4);
		assert(memcmp(buf, zero_buf, 4) == 0);
		assert(memcmp(buf+4, writes[i-1][0].buf, 
					  writes[i-1][0].len-4) == 0);
	}

	// at this point, any pread beyond 4 should return 0
	off_t off;
	size_t s;
	for(i = 0; i < 10; i++) {
		off = rand() % (EXPLODE_OFF_MAX-sizeof zero_buf);
		if(off < 4) continue;
		s = rand()%(sizeof zero_buf);
		d.pread(buf, s, off);
		assert(memcmp(buf, zero_buf, s) == 0);
	}

	d.clear();
	cout<<"test mixed write\n";
	char mixed_buf[1024];
	for(i = 1; i < nwrites; i++) {
		memset(mixed_buf, 0, sizeof mixed_buf);
		memcpy(mixed_buf+16, writes[i][0].buf, writes[i][0].len);
		d.pwrite(mixed_buf, sizeof mixed_buf, writes[i][0].off-16);
		d.print(cout);
		d.internal_check();
	}
	for(i = 1; i < nwrites; i++) {
		d.pread(mixed_buf, sizeof mixed_buf, writes[i][0].off-16);
		assert(memcmp(mixed_buf+16, writes[i][0].buf, writes[i][0].len)
			   == 0);
		memset(mixed_buf+16, 0, writes[i][0].len);
		assert(memcmp(mixed_buf, zero_buf, sizeof mixed_buf)==0);
	}

	// test data write
	d.clear();
	for(i = 0; i < num; i++) {
		drs[i].magic = 0;
		drs[i].start = (i+1) * 100+1;
		drs[i].end = drs[i].start + 40+3;
		drs[i].data = new char[drs[i].end-drs[i].start];
		assert(drs[i].data);
		memset(drs[i].data, i+100, drs[i].end-drs[i].start);
		cout << "region " << i << " ["
			 << drs[i].start << ", " << drs[i].end << ")\n";
		//d[drs[i].start] = drs[i].clone();
	}

	cout<<"test pwrite then pread\n";
	for(i = 0; i < num; i++) {
		memset(mixed_buf, 0, sizeof mixed_buf);
		memcpy(mixed_buf+24, drs[i].data, drs[i].end-drs[i].start);
		d.pwrite(mixed_buf, sizeof mixed_buf, drs[i].start-24);
		cout << "after write " << i << ":\n";
		d.print(cout);
		d.pread(mixed_buf, sizeof mixed_buf, drs[i].start-24);
		assert(memcmp(mixed_buf+24, drs[i].data, 
					  drs[i].end-drs[i].start) == 0);
	}

	char big_buf[2048];
	d.pread(big_buf, sizeof big_buf, 0);
	for(i = 0; i < num; i++) {
		assert(memcmp(big_buf+drs[i].start, 
					  drs[i].data, drs[i].end-drs[i].start) == 0);
		memset(big_buf+drs[i].start, 0, drs[i].end-drs[i].start);
	}

	for(i = 0; i < sizeof big_buf; i++)
		assert(!big_buf[i]);

	cout<<"test truncate\n";
	off_t length;
	for(i = num-1; i >= 0; i--) {
		length = (drs[i].start + drs[i].end)/2;
		d.truncate(length);
		cout <<"after tuncate to " << length << ":\n";
		d.print(cout);
		d.pread(buf, drs[i].end-drs[i].start+15, drs[i].start-15);
		assert(memcmp(buf, zero_buf, 15) == 0);
		assert(memcmp(buf+15, drs[i].data, length-drs[i].start) == 0);
		assert(memcmp(buf+15+length-drs[i].start, 
					  zero_buf, drs[i].end-length) == 0);
	}
	length = drs[0].start;
	d.truncate(length);
	cout<<"after truncate to " << length << ":\n";
	d.print(cout);

	// at this point, any pread should return 0
	for(i = 0; i < 10; i++) {
		off = rand() % (EXPLODE_OFF_MAX-sizeof zero_buf);
		s = rand()%(sizeof zero_buf);
		d.pread(buf, s, off);
		assert(memcmp(buf, zero_buf, s) == 0);
	}
	
	d.clear();
	char magic_buf[1024];
	off = 4200;
	for(i = 0; i < sizeof magic_buf; i+=ALIGN) {
		signature_t sig = position_hash(off+i, position_magic[1]);
		memcpy(magic_buf+i, &sig, sizeof(sig));
	}
	d.pwrite(magic_buf, sizeof magic_buf, off);
	d.print(cout);

	for(i = 1; i < sizeof magic_buf; i+=ALIGN) {
		char c = 0;
		d.pwrite(&c, 1, off+i);
		cout << "write to " << off+i << ":";
		d.print(cout);
	}
	cout << "finally:";
	d.print(cout);

	for(i = 1; i < sizeof magic_buf; i+=ALIGN) {
		char c = magic_buf[i];
		d.pwrite(&c, 1, off+i);
		cout << "write to " << off+i << ":";
		d.print(cout);
	}
	cout << "finally:";
	d.print(cout);

	for(i = sizeof magic_buf; i >= 0; i--) {
		d.truncate(off+i);
		cout << "tuncate to " << off+i << ":";
		d.print(cout);
	}		
}

#endif
