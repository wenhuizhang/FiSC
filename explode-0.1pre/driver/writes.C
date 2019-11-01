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


#include "writes.h"
#include <iostream>
#include <iterator>
#include <algorithm>

using namespace std;

#if 0
write_t writes[NUM_WRITES] = {
	{0, 1*sizeof(signature_t)},
	{1023, 2*sizeof(signature_t)},
	{4093, 3*sizeof(signature_t)},
	{MEDIUM_FILE_LEN-13, 5*sizeof(signature_t)},
	{BIG_FILE_LEN-31, 10*sizeof(signature_t)},
};
#else
// word aligned
write_t writes[NUM_WRITES][nposition_magic] = {
	{
		{0, 1*sizeof(signature_t)},
		{0, 1*sizeof(signature_t)},
	},
	{
		{1020, 2*sizeof(signature_t)},	
		{1020, 2*sizeof(signature_t)},
	},
	{
		{4092, 3*sizeof(signature_t)},
		{4092, 3*sizeof(signature_t)},
	},
	{
		{MEDIUM_FILE_LEN-12, 5*sizeof(signature_t)},
		{MEDIUM_FILE_LEN-12, 5*sizeof(signature_t)},
	},
	{
		{BIG_FILE_LEN-32, 10*sizeof(signature_t)},
		{BIG_FILE_LEN-32, 10*sizeof(signature_t)},
	},
};
#endif
int nwrites = sizeof writes/sizeof writes[0];

signature_t position_hash(off_t o, int magic) 
{
	char buf[sizeof o + sizeof magic];
	memcpy (buf, &o, sizeof o);
	memcpy (buf + sizeof o, &magic, sizeof magic);
	return explode_hash((ub1*)buf, sizeof o + sizeof magic);
}

void init_writes (void)
{
	static bool init = false;
	int i, j, k;

	if(init) return;

	init = true;

	for(k = 0; k < nposition_magic; k++)
		assert(position_magic[k]);

	for(i = 0; i < nwrites; i++) {
		for(k = 0; k < nposition_magic; k++) {
			writes[i][k].size = writes[i][k].off 
				+ writes[i][k].len;
			char *p = writes[i][k].buf;
			memset(p, 0, sizeof(writes[i][k].buf));
			int nsigs = writes[i][k].len/sizeof(signature_t);
			
			assert(nsigs <= MAX_SIG_WRITE);
			for(j=0; j < nsigs; j++) {
				signature_t sig;
				sig = position_hash(writes[i][k].off+sizeof sig*j,
									position_magic[k]);
				memcpy(p, (char*)&sig, sizeof sig);
				p += sizeof sig;
			}
#if 0		
			write_t *w = writes + i;
			w->sig = 0;
			for(j=0; j<=i; j++) {
				write_t *wj = writes + j;
				w->sig = hash3((ub1*)wj->buf, wj->len, w->sig);
			}
#endif
		}
	}

#ifndef DEBUG
	cout << "pre-defined file hashes\n";
	for(i = 0; i < nwrites; i++) {
		for(k = 0; k < nposition_magic; k++) {
			cout <<"size=" << writes[i][k].size << ";";
			// << "sig=" << writes[i][k].sig << ";";

			cout <<"buf=";
			for(j=0; j < (int)writes[i][k].len; j++)
				cout<<to_hex(writes[i][k].buf[j]) << " ";
			cout << endl;
		}
	}
#endif
}

// return next write.  
unsigned next_write_by_size(off_t size)
{
	int i = 0;
	while(size >= writes[i][0].size && i < nwrites)
		i++;
	return i;
}

signature_t hash_file(const char* path, struct stat &st)
{
	if(st.st_size == 0) return 0;

	int i = next_write_by_size(st.st_size);
#ifndef DEBUG
	if(i > nwrites) {
		cout << "size = " << st.st_size << endl;
		error("inconsistent", "file size can never be that lareg!");
	}
#endif

	int fd = open(path, O_RDONLY);
	CHECK_RETURN(fd);

	int ret, j;
	char buf[MAX_SIG_WRITE * sizeof(signature_t)];
	signature_t sig = 0;
	for(j = 0; j < i; j ++) {
		int len;
		if(j == i-1) 
			//FIXME. if truncate or write is half
			//done, how to check?
			len = st.st_size - writes[i-1][0].off;
		else 
			len = writes[j][0].len;
		ret = ::pread(fd, buf, len, writes[j][0].off);
		CHECK_RETURN(ret);
		sig = hash3((ub1*)buf, len, sig);
#ifdef DEBUG
		copy(buf, buf+len, 
		     std::ostream_iterator<int>(cout, ","));
		std::cout << endl;//std::cout << sig << endl;
#endif
	}
	
	close(fd);
	return sig;
}

