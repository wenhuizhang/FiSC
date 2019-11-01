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


#include "Archive.h"
#include "mcl.h"
#include "lookup2.h"

using namespace std;
void Archive::init(unsigned  l) {
    len = (l < ARCHIVE_MIN_LEN?ARCHIVE_MIN_LEN:l);
    cur_rd = cur_wr = buf = (char*)malloc(len);	
    xi_assert(buf, "out of mem!");
}

void Archive::check_expand(unsigned l) {
    // should prbabaly be more efficient on memory
    while(cur_wr + l  > buf + len)
        resize (len << 1);
}

void Archive::put(const char* p, unsigned l) {
    check_expand(l);
    memcpy (cur_wr, p, l);
    cur_wr += l;
}

void Archive::get(char *p, unsigned l)
{
    xi_assert(cur_rd + l <= cur_wr, 
              "reads uninitialized memory");
    memcpy(p, cur_rd, l);
    cur_rd += l;
}

void Archive::put(string& s)
{
    string::size_type l = s.length();
    (*this) << l;
    this->put((const char*)s.c_str(), l);
}

void Archive::get(string& s) {
    string::size_type l;
    (*this) >> l;
    char *t = new char[l];
    assert(t);
    get(t, l);
    s.clear();
    s.append(t, l);
    delete[] t;
}

signature_t Archive::get_sig(void) {
#if 0
    int len = cur_wr - buf;
    for (int i = 0; i < len; i ++) {
        printf("%2x", (unsigned int)(buf[i]));
    }
    printf("\n");
#endif
    return explode_hash((ub1*)buf, cur_wr - buf);
}

void Archive::attach(char *b, int m)
{
    if(buf) free(buf);
    buf = b;
    len = m;
    cur_wr = buf + m;
    cur_rd = buf;
}

char* Archive::detach(int &m)
{
    char* p = buf;
    m = len;
    len = 0;
    buf = cur_wr = cur_rd = cur_rd = NULL;
    return p;
}

void Archive::resize(int l)
{
    assert (l >= (cur_wr - buf));
    len = l;
    char *old = buf;
    buf = (char*)realloc (buf, l);
    if (!buf) {
        free(old);
        xi_panic("out of memory");
    }
    cur_wr += buf - old;
    cur_rd += buf - old;
}
