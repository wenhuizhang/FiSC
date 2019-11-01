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


/* -*- mode: C++ -*- */
#ifndef __RANDOM_H
#define __RANDOM_H

#include <vector>
#include "State.h"

class Random
{
 public:
	virtual void set_choices (trace_t &choices)	{
		_choices = choices;
	}
		
	virtual int random(int ceil) {return 0;}
	int random_fail(void) {return this->random(2);}
	virtual void reset(void) {}
	virtual bool repeat(void) {return false;}
	virtual ~Random() {}

	trace_t _choices;
};

class UnixRandom: public Random
{
public:
	virtual int random(int ceil);
	virtual void reset(void);
};

class ReplayRandom: public Random
{
protected:
	int _cur;
public:
	ReplayRandom() {_cur = 0;}
	virtual int random(int ceil);
	virtual void reset(void);
	virtual bool repeat(void);
};

class EnumRandom: public Random
{
protected:
	std::vector<choice_t> _ceils;
	int _cur; // index into the current choice point. starting
		  // from -1
public:
	EnumRandom() {_cur = 0;}
	virtual int random(int ceil);
	virtual void reset(void);
	virtual bool repeat(void);

	void skip_rest();
};

enum {UNIX_RAND, ENUM_RAND, REPLAY_RAND};

Random *new_random (int rand_type);

#endif
