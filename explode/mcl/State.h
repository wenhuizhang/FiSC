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


/* 
   Each state includes
   1. checkpoint, which should be just a disk
   2. trace from the checkpoint to this state

   trace records all the choices.
 */

#ifndef __STATE_H
#define __STATE_H

#include<vector>
#include "mcl_types.h"

class State {
public:
	int start; // for now, this is always 0
	int id;
	signature_t sig;
	trace_t trace;

	void to_file(int fd);
	State() {}
	State(int s, int i, signature_t si)
		: start(s), id(i), sig(si) {}
	State(int fd);
};
std::ostream& operator<<(std::ostream& o, const State& st);

class ScoreState: public State {
public:
	int score;

	void to_file(int fd);
	ScoreState() {}
	ScoreState(int s, int i, signature_t si, int sc)
		: State(s, i, si), score(sc) {}
	ScoreState(int fd);
	
	bool operator<(ScoreState& s) {
		return score < s.score;
	}
};
std::ostream& operator<<(std::ostream& o, const ScoreState& st);

struct Checkpoint {
	char* data; // checkpointed data
	int len; // checkpoint length
	int parent_id; //its parent checkpoint
	trace_t trace; // how to get to this checkpoint from its parent

	void to_file(int fd);
	Checkpoint() {}
	Checkpoint(int fd);
};
std::ostream& operator<<(std::ostream& o, const Checkpoint& cpkt);

#endif

