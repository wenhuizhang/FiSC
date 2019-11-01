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
  It makes perfect sense to use a virtual class for state queue
  implementation.  We can plug in different queues (dynamic
  dispatch) without changing the type declaration for the state
  queue inside model checker, thusly avoid re-compiling large
  trunk of irrelevant code. Sure you can use void* or a struct
  with function pointers, but those ways are ad-hoc.
*/

#ifndef __STATEQUEUE_H
#define __STATEQUEUE_H

#include <iostream>
#include <list>
#include <iterator>

#include <assert.h>

#include "util.h"
#include "State.h"

class StateQueue {
public:
	virtual bool empty() const = 0;
	virtual int size() const = 0;
	virtual State* top() const = 0;
	virtual State *pop() = 0;
	virtual void push(State *s) = 0;

	virtual std::ostream& print(std::ostream& o) const {};
	virtual void save_states_to_file(int fd) const = 0;
	virtual void restore_states_from_file(int fd) = 0;
	virtual ~StateQueue() {}
};

// this iterator thing is stupid
template<class T>
class StateQueueImpl: public StateQueue {
protected:
	std::list<T*> _q;

public:
	bool empty() const {return _q.empty();}
	int size () const {return _q.size();}
	State* top() const {return dynamic_cast<State*>(_q.front());}
	State* pop() {
		T *s = _q.front();
		_q.pop_front();
		return dynamic_cast<State*>(s);
	}

	std::ostream& print(std::ostream& o) const {
		typename std::list<T*>::const_iterator it;
		o << "state queue size " << size() << "\n";
		for_all_iter(it, _q)
			o << *(*it) << "\n";
		return o;
	}

	void save_states_to_file(int fd) const {
		typename std::list<T*>::const_iterator it;
		int sz = size();
		write(fd, &sz, sizeof sz);
		for_all_iter(it, _q)
			(*it)->to_file(fd);
	}
	
	void restore_states_from_file(int fd) {
		int sz;
		read(fd, &sz, sizeof sz);
		while(sz--)
			_q.push_back(new T(fd));
	}

	~StateQueueImpl() {
		typename std::list<T*>::iterator it;
		for_all_iter(it, _q)
			delete *it;
		_q.clear();
	}
};

class BFSQueue: public StateQueueImpl<State> {
public:
	void push (State *s) {_q.push_back(s);}
};

class DFSQueue: public StateQueueImpl<State> {
public:
	void push(State *s) {_q.push_front(s);}
};

class PrioQueue: public StateQueueImpl<ScoreState> {
public:
	typedef std::list<ScoreState*>::iterator iterator;
	typedef std::list<ScoreState*>::const_iterator const_iterator;
	iterator begin() {return _q.begin();}
	iterator end() {return _q.end();}
	void push(State *s) {
		ScoreState* ss = static_cast<ScoreState*>(s);
		iterator it;
		it = lower_bound(_q.begin(), _q.end(), ss);
		_q.insert(it, ss);
	}
};

std::ostream& operator<<(std::ostream& o, const StateQueue& q);

#endif
