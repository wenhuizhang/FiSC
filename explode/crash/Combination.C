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

#include "Combination.h"

//#define DEBUG
#ifdef DEBUG
#	include <cstdlib>
#	define unix_random(ceil)  \
		((int) (((double)ceil)*::random()/(RAND_MAX+1.0)))
#else
#	include "mcl.h"
#endif

using namespace std;

void Combination::rand_subset(set_t& subset)
{
	int i;

	subset.clear();
	for(i = 0; i < (int)ceils.size(); i++)
		subset.push_back(unix_random(ceils[i]+1));
}

void Combination::reset(void)
{
    iter = 0;
}

bool Combination::next(void)
{
	int i;
#if 0
	if(!ceils.size())
		return false;
#endif

	if((int)ceils.size() > deterministic_threshold) {
		if(iter > random_tries)
			return false;
		vv_out << "use random combinations" << endl;
		rand_subset(subset);
		goto return_true;
	}		

	// total <= deterministic_threshold
	if(!iter) {
		// first iteration
		subset.clear();
		for(i = 0; i < (int)ceils.size(); i++)
			subset.push_back(0);

		goto return_true;
	}

	// later iterations
	assert(subset.size() == ceils.size());
	for(i = (int)subset.size() - 1; i >= 0; i --) {
		subset[i] ++;
		if(subset[i] > ceils[i]) {
			subset[i] = 0;
		} else 
			break;
	}

	// i < 0 means we've iterated all combinations
	if (i < 0) {
		assert(iter == total);
		return false;
	}

 return_true:
	iter ++;
	return true;
}

Combination::Combination(int t, int deter_thresh, int rand_tries)
    : type(t), 	// type is ignored for now
      deterministic_threshold(deter_thresh),
      random_tries(rand_tries)
{}

Combination::Combination(set_t& cl, int t, 
                         int deter_thresh, int rand_tries)
    : type(t), 	// type is ignored for now
      deterministic_threshold(deter_thresh),
      random_tries(rand_tries)
{
    ceilings(cl);
}



void Combination::ceilings(set_t& cl)
{
    set_t::iterator it;
    // shouldn't have 0s in ceils
    for_all_iter(it, cl)
        assert(*it);
    
    ceils = cl;

    total = 1;
    for_all_iter(it, ceils)
        total *= ((*it) + 1);
    
    if(total < 0) // total can overflow...
        total = 1000000;
    //assert(total > 1);
    
    iter = 0;
}

std::ostream& operator<<(std::ostream& o, const Combination::set_t subset)
{
	copy(subset.begin(), subset.end(), ostream_iterator<int>(o, ","));
	return o;
}

#ifdef DEBUG
int main(void)
{
	const unsigned N = 10;
	vector<int> v;
	Combination c;

	v.push_back(1);
	c.init(v, 0);
	while(c.next()) {
            set_t& subset = *c;
            copy(subset.begin(), subset.end(),
                 ostream_iterator<int>(cout, " "));
            cout<<endl;
	}

	v.push_back(1);
	v.push_back(3);
	v.push_back(2);
	c.init(v, 0);
	while(c.next_subset(subset)) {
		copy(subset.begin(), subset.end(),
		     ostream_iterator<int>(cout, " "));
		cout<<endl;
	}

}
#endif
