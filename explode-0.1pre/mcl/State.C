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


#include <iterator>

#include "util.h"
#include "mcl.h"
#include "State.h"

using namespace std;

static void write_trace(int fd, trace_t &tr)
{
	int size = tr.size(), i;
	write(fd, &size, sizeof size);
	for(i = 0; i < (int)tr.size(); i++)
		write(fd, &tr[i], sizeof tr[i]);
}

static void read_trace(int fd, trace_t &tr)
{
	int size, i;
	choice_t c;
	read(fd, &size, sizeof size);
	for(i = 0; i < size; i++) {
		read(fd, &c, sizeof c);
		tr.push_back(c);
	}
}

void State::to_file(int fd)
{
	write(fd, &this->id, sizeof this->id);
	write(fd, &this->start, sizeof this->start);
	write(fd, &this->sig, sizeof this->sig);
	write_trace(fd, this->trace);
}

State::State(int fd)
{
	read(fd, &this->id, sizeof this->id);
	read(fd, &this->start, sizeof this->start);
	read(fd, &this->sig, sizeof this->sig);
	read_trace(fd, this->trace);
}

ostream& operator<<(ostream& o, const State& st)
{
	o << "state " << st.id << ": ";
	copy(st.trace.begin(), st.trace.end(),
		 ostream_iterator<int>(o, " "));
	o << " length " << st.trace.size();
	return o;
}

void ScoreState::to_file(int fd)
{
	State::to_file(fd);
	write(fd, &this->score, sizeof this->score);
}

ScoreState::ScoreState(int fd): State(fd) 
{
	read(fd, &this->score, sizeof this->score);
}

ostream& operator<<(ostream& o, const ScoreState& st)
{
	o << static_cast<const State&>(st) << " score " << st.score;
	return o;
}


void Checkpoint::to_file(int fd)
{
	write(fd, &this->len, sizeof this->len);
	write(fd, this->data, this->len);
	write(fd, &this->parent_id, sizeof this->parent_id);
	write_trace(fd, this->trace);
}

Checkpoint::Checkpoint(int fd)
{
	read(fd, &this->len, sizeof this->len);
	this->data = new char[this->len];
	assert(this->data);
	read(fd, this->data, this->len);
	read(fd, &this->parent_id, sizeof this->parent_id);
	read_trace(fd, this->trace);
}
