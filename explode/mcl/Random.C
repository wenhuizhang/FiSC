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


#include "Random.h"
#include <assert.h>
#include <iostream>
#include <cstdlib>
#include "mcl.h"

//#define DEBUG_RANDOM
using namespace std;

/**************************************************************/
/* PureRandom is identical to unix random except we record
   our decisions. Useful for doing pure simulation or debuging */
int UnixRandom::random(int ceil)
{
	assert(ceil <= MAX_CEIL);
	int choice = (int) (((double)ceil)*::random()
			    /(RAND_MAX+1.0));

	// record choice here
	_choices.push_back(choice);
	return choice;	
}

void UnixRandom::reset(void)
{
	_choices.clear();
	srand(get_option(mcl, random_seed));
}


/**************************************************************/
/* ReplayRandom is for replaying traces */
#include <iostream>
int ReplayRandom::random(int ceil)
{
	assert(ceil>0 && ceil<=MAX_CEIL);

	if(ceil == 1) return 0;

	if((unsigned)_cur >= _choices.size()) {
		cout << "how come? total " << _choices.size() << " choices, "
			 << "need more than " << _cur << "choices" << endl;
	}
	if(_choices[_cur] >= ceil) {
		cout << "ReplayRandom error! choice " << (int)_choices[_cur] 
		     << " is beyond ceiling " << ceil << endl;
	}
	assert(_cur >= 0 && (unsigned)_cur < _choices.size()
		   && _choices[_cur] < ceil);
	return _choices[_cur++];
}

void ReplayRandom::reset(void)
{
	_cur = 0;
}

bool ReplayRandom::repeat(void)
{
	return _cur < (int)(_choices.size());
}


/**************************************************************/
/* Enumerate all possible choices for all choice points, starting
   from the last choice point */
/* TODO: should record ceilings, so we can do sanity check when repeating */
int EnumRandom::random(int ceil)
{
	assert(ceil>0 && ceil<=MAX_CEIL);
#if 1
	// XXX Why not just return ???
	// need to record choice for replaying purpose
	if (ceil == 1) return 0;
#endif
	
	_cur ++;

	// if this is an old choice point, return the choice value
	if (_cur < ((int)_ceils.size()))
		return _choices[_cur];

	// otherwise add this choice point to choice stack
	_ceils.push_back(ceil);
	_choices.push_back(0);
	return 0;
}

// set the last choice point to be at the last value
void EnumRandom::skip_rest()
{
	cout << _cur << endl;
	assert(_cur >= 0
	       && _cur < ((int)_ceils.size()));
	if(_ceils[_cur] <= 1)
		return;
	_choices[_cur] = _ceils[_cur]-1;
}

void EnumRandom::reset(void)
{
	_cur = -1;
	_ceils.clear();
	_choices.clear();
}

bool EnumRandom::repeat(void)
{
	/* when repeat is called, increase the last choice value
	   by 1. if it reaches its ceiling, increase the value for
	   the previous choice point.  keep doing this until we
	   find a choice point where its value has not reach its
	   ceiling yet, or all the choice values are explored for
	   all choice points*/
	while (_cur >= 0) {
		_choices[_cur] ++;
		if (_choices[_cur] < _ceils[_cur])
			break;
		_cur --;
	}
	
	// all choice points are explored.  stop repeating
	if (_cur < 0)
		return false;
	
	// not all choice points are explored, repeat.  resize
	// _choices and _ceils here to only contain valid choice
	// points
	_choices.resize (_cur+1);
	_ceils.resize(_cur+1);
	_cur = -1;
	return true;
}

/**************************************************************/
/* Choice graph is essentially a partial tree (or a partial DAG).
   Some choices are explored, corresponding to expanded subgraph;
   Some choices aren't explored yet.  We want to have control over
   how we search this tree */
class TreeRandom: public Random
{
public:
	virtual int random(int ceil);
	virtual void reset(void);
	virtual bool repeat(void);
};


/**************************************************************/
Random *new_random (int rand_type)
{
	Random *r = NULL;
	switch (rand_type) {
	case UNIX_RAND:
		r = new UnixRandom;
		break;
	case ENUM_RAND:
		r = new EnumRandom;
		break;
	case REPLAY_RAND:
		r = new ReplayRandom;
		break;
	}
	assert(r);
	return r;
}

#ifdef DEBUG_RANDOM
#include <iostream>

int main(void) {
	int i=0, j=0, k=0, n = 0;
	Random *r = new_random (ENUM_RAND);
	r->reset();
	do {
		i = r->random(16);
		j = r->random(i);
		k = r->random(j);
		cout << "(" 
		     << i << ", "
		     << j << ", " 
		     << k << ")" << endl;
		n ++;
	} while (r->repeat());
	cout << "total " << n << endl;
	delete r;

	// the above loop should iterate as many times as the
	// below loop + 16 (for the cases where j == k == 0)
	for (i = 0; i < 16; i ++) 
		for (j = 0; j < i; j ++)
			for (k = 0; k < j; k++) {
				cout << "(" 
				     << i << ", "
				     << j << ", " 
				     << k << ")" << endl;
				n --;
			}
	cout << "n = " << n << endl;
	assert (n == 16);
	return 1;
}
#endif
