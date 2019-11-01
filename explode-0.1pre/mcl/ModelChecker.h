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


#ifndef __MODELCHECKER_H
#define __MODELCHECKER_H

#include "mcl.h"
#include "StateQueue.h"
#include "Random.h"

#include "hash_map_compat.h"
#include <vector>

typedef Sgi::hash_map<int, Checkpoint*> ckpt_tbl_t;
typedef Sgi::hash_map<int, State*> state_tbl_t;
typedef Sgi::hash_map<signature_t, int> visited_tbl_t;
typedef Sgi::hash_map<int, int> score_tbl_t;
typedef void (TestHarness::*xe_user_func_t)(void);

// model checker mode
enum {
    MODE_MODEL_CHECK = 1,
    MODE_REPLAY = 2,
    MODE_REGEN_STATE = 3,
};

class ModelChecker {

private:

    TestHarness *_test;

    StateQueue *_q;

    // id -> checkpoint map.  Each checkpoint captures an
    // execution context.  

    //TODO 1. free a ckpt if no states in the current queue
    //refers to a checkpoint, we can free it.  perfect to use
    //a smart pointer here 2. need to store a trace from a
    //reachable checkpoint to each checkpoint
    ckpt_tbl_t _ckpts;

    // sig -> id table, which records all visisted states
    visited_tbl_t _visited_states;

    // id -> state table
    // FIXME: with _ckpts, state queue and _visited_states, 
    // no longer need this table.
    // state_tbl_t _states;

    // FIXME: _states should go away.  this should also go away.
    // merge them into StateQueue
    // state id -> score table
    // score_tbl_t _scores;

    int _last_score;

    // 1: model checking 
    // 2: trace replaying
    // 3. regenerating state during model checking
    int _mode;

    // number of bugs found
    int _bugs;

    int _restored;

    int _mon_skt;

    bool _need_restart;

    xe_user_func_t _last_user_func;
	
    // random
    Random *_rand;
    // this is simply unix random, except we log the results
    // that are returned.
    Random *_unix_rand;

    // current state
    State *_curr;

    void clear(void);
    void end_model_check(void);
    void replay(void);
    int run_test(void (TestHarness::*f)(void));
    int run_new_transition(void);

    void print(std::ostream& o, bool verbose);

    void set_queue(void);

	
    State* new_state(signature_t sig);
    void take_ckpt(State *parent, State* cur);
    State* init_state(void);


    // need id because we don't store a State object for for expanded states
    State* get_state(State *parent, int& id);
    void set_state (State *);

    bool seen(signature_t);

    void store_trace(void);
    void store_trace_file(void);
    void load_trace_file(const char* filename, trace_t& old_trace,
                         trace_t& new_trace);

    void status(void);
    void process_test_return (int, bool);

    void self_checkpoint(const char* file);
    void connect_to_monitor(void);
    void heart_beat(void);

public:
    ModelChecker(int mode);
    ~ModelChecker();
    Random *get_rand() const {return _rand;}
    Random *get_unix_rand() const {return _unix_rand;}
    int mode() const {return _mode;}
    // set score for current state
    void set_score(int score);
    void check_valid_choose(void);

    void model_check(void);
    // main function to replay a trace contained in @filename
    void replay_trace_file(const char* filename);

    const char* trace_dir(void);

    static ModelChecker* self_restore(const char* file);
    void restart(int how); // tell the monitor to restart

    // total states created
    int num_states(void) const {return _visited_states.size();}
    // total live states. i.e. states that are on state queue
    int num_states_in_queue(void) const {return _q->size();}

    void register_test(TestHarness *test) {_test = test;}
};


#ifdef CONF_USE_EXCEPTION
#include <string>
class CheckerException {
public:
    int code;
    std::string msg;
    CheckerException(int c, const char* m) {
        code = c;
        msg = m;
    }
};
#endif

extern ModelChecker *explode;
#endif
