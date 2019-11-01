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


#include<vector>
#include<iterator>
#include<iostream>
#include<algorithm>
#include<assert.h>

#include"Powerset.h"

//#define DEBUG
#ifdef DEBUG
#	include <cstdlib>
#	define unix_random(ceil)  \
		((int) (((double)ceil)*::random()/(RAND_MAX+1.0)))
#define EXPLODE_EXTRA_VERBOSE (0)
#else
#	include "mcl.h"
#endif

using namespace std;

bool Powerset::generated = false;
std::vector<Powerset::set_t> Powerset::powerset(1<<MAX_SET);
/* given @n, generates powerset @powerset for {1, 2, ..., n-1}

Subsets are ordered in a way that
0..(2^i-1) are are subsets of {1, 2, ..., i-1}, and
0..(2^(i+1)-1) are subsets of {1, 2, ..., i} that contains i */
void Powerset::gen_powerset_(unsigned n, unsigned& s, vector<set_t>& powerset)
{
    if(n == 0) {
        s = 1;
        return;
    }
    gen_powerset_(n - 1, s, powerset);
    for(unsigned i = 0; i < s; i ++) {
        set_t st;
        powerset[i+s] = powerset[i];
        powerset[i+s].insert(n-1);
    }
    s = s<<1;
}

void Powerset::gen_powerset(void)
{

    if(generated) return;
    generated = true;

    unsigned s;

    gen_powerset_(MAX_SET, s, powerset);

#if 0
    if(EXPLODE_EXTRA_VERBOSE) {
        cout<<"power sets\n";
        for(int i = 0; i < (1<<MAX_SET); i++) {
            copy(powerset[i].begin(), powerset[i].end(), 
                 ostream_iterator<int>(cout, " "));
            cout<<endl;
        }
    }
#endif
}

void Powerset::rand_subset(set_t& subset)
{
    int i;
    vector<int> rand_set(size);
    int sz = unix_random(size);

    for(i = 0; i < size; i++)
        rand_set[i] = i;
	
    for(i = 0; i < sz; i++) {
        unsigned j = unix_random(size-i);
        int t = rand_set[j];
        rand_set[j] = rand_set[i];
        rand_set[i] = t;
    }
    rand_set.resize(sz);
    for(i = 0; i < (int)rand_set.size(); i++)
        subset.insert(rand_set[i]);
}

bool Powerset::next(void)
{
    int i;

    gen_powerset();

    if(!iter)
        subset.clear();

    assert(powerset.size() == 1<<MAX_SET);

    switch(type) {
    case atomic:
        if(iter == 1)
            return false;
        for(i = 0; i < size; i ++)
            subset.insert(i);
        break;
		
    case left_to_right:
        if(iter > size)
            return false;
        if(iter == 0)
            break;  // empty set
        else
            subset.insert(iter-1);
        break;

    case unordered:
        if(size <= deterministic_threshold) {
            // skip
            while(skip_table.test(iter) && iter < (1<<size)) {
                cout<<"Powerset skip " << powerset[iter] << endl;
                iter++;
            }

            if(iter == (1<<size))
                return false;
            subset = powerset[iter];
        } else {
            if(iter > random_tries)
                return false;
            vv_out << "use random subsets" << endl;
            rand_subset(subset);
        }
        break;
    }
    iter++;
    return true;
}

static int find_order(int x)
{
    int order = 0;
    int t = 0;

    if(x&(x-1))
        t = 1;

    while(x > 1) {
        order ++;
        x = x>>1;
    }
   
    return order+t;
}

void Powerset::skip_prefix(void)
{
    int skip_base = iter-1;
    int order = find_order(iter);
    int skip_distance = 1<<order;

    while(skip_base < (1<<size)) {
        skip_table.set(skip_base);
        skip_base += skip_distance;
    }
}

ostream& operator<<(ostream& o, const Powerset::set_t subset)
{
    copy(subset.begin(), subset.end(), 
         ostream_iterator<int>(o, " "));
    return o;
}

#ifdef DEBUG
int main(void)
{
    const unsigned N = 10;
    Powerset::gen_powerset();
    Powerset p;
    Powerset::set_t subset;

#if 0
    p.init(1, unordered);
    while(p.next_subset(subset)) {
        copy(subset.begin(), subset.end(),
             ostream_iterator<int>(cout, " "));
        cout<<endl;
    }
#endif
    Powerset::set_t::iterator it;
    int i = 0;
    p.init(5, unordered, 1000000, 1000000);
    while(p.next_subset(subset)) {
#if 1
        if(subset.size() == 3) {
            int x1, x2, x3;
            it = subset.begin();
            x1 = (*it);
            it++;
            x2 = (*it);
            it++;
            x3 = (*it);
            it++;
            if(x1==0 && x2==1 && x3==2)
                p.skip_prefix(subset);
        } else if(subset.size() == 2) {
            int x1, x2;
            it = subset.begin();
            x1 = (*it);
            it++;
            x2 = (*it);
            if(x1==1 && x2==3)
                p.skip_prefix(subset);
        } else if(subset.size() == 1) {
            int x1;
            it = subset.begin();
            x1 = (*it);
            if(x1==4)
                p.skip_prefix(subset);
        }
#endif
        cout << i++ << ": ";
        copy(subset.begin(), subset.end(),
             ostream_iterator<int>(cout, " "));
        cout<<endl;
    }
}
#endif
