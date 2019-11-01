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


#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>
#include <list>
#include <sstream>

#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#include "mcl.h"
#include "ModelChecker.h"
#include "Monitor.h"

// abort mechanism isn't clean.  re-think

//#define DEBUG
#define EAGER_DELETE_STATE

#define STATE_HAS_SCORE (option_eq(mcl, state_queue, "priority"))

using namespace std;

ModelChecker *explode = NULL;

#define persistent (get_option(mcl, persistent) != 0)

ostream& operator<<(ostream& o, const trace_t& t)
{
    copy(t.begin(), t.end(), ostream_iterator<int>(o, " "));
    return o;
}

ModelChecker::ModelChecker(int mode)
{
    _test = NULL;
    _mode = mode;

    switch(_mode) {
    case MODE_MODEL_CHECK:
        if(get_option(mcl, simulation))
            _rand = new_random(UNIX_RAND);
        else
            _rand = new_random(ENUM_RAND);
        break;

    case MODE_REPLAY:
        _rand = new_random(REPLAY_RAND);
        break;
    default:
        assert(0);
    }

    _unix_rand = new_random(UNIX_RAND);
    set_queue();
    _bugs = 0;
    _restored = 0;
    _mon_skt = -1;
    _need_restart = false;
    _last_user_func = NULL;
    _last_score = 0;
}

ModelChecker::~ModelChecker()
{
    clear();
}

void ModelChecker::clear() 
{
    if (_q) delete _q;
    if (_rand) delete _rand;
    if(_unix_rand) delete _unix_rand;

    ckpt_tbl_t::iterator it;
    for_all_iter(it, _ckpts) {
        Checkpoint *ckpt = (*it).second;
        delete [] ckpt->data;
        delete ckpt;
    }

    if(_mon_skt > 0)
        close(_mon_skt);
}

void ModelChecker::end_model_check(void)
{
    if(_mode == MODE_MODEL_CHECK && persistent) {
        // TODO should checkpoint here too
        Monitor::send(_mon_skt, Monitor::prog_done);
    }
    _test->exit();
    clear();
    exit(0);
}

// tell the monitor to restart. 
// how = 0: schedule a restart.  will
// actually restart next time we come back to the model checking
// loop, when the internal state is consistent.
// how = 1: take a checkpoint then restart now
// how = 2: restart immediately
void ModelChecker::restart(int how)
{
    status_out << "restart called with how =" << how << endl;
    // XXX. no longer check the current state upon
    // reboot. dunno if this is correct
    status_out << "restarting. total # of states "
               << num_states() << endl;
    status_out << flush;

    if(how == 0) { // schedule one
        _need_restart = true;
        return;
    } 

    if(how == 1)
        // take a checkpoint before restart
        self_checkpoint(get_option(mcl, ckpt_file));
	
    sync();
    Monitor::send(_mon_skt, Monitor::want_restart);
    close(_mon_skt);
}

void ModelChecker::heart_beat(void)
{
    stringstream ss;
    print(ss, false);
    string str = ss.str();

    Monitor::send_heart_beat(_mon_skt, str.c_str(), str.length());
}

void ModelChecker::connect_to_monitor(void)
{
    const char* addr = get_option(monitor, addr);
    int port = get_option(monitor, port);
	
    _mon_skt = Monitor::connect(addr, port);
    assert(_mon_skt >= 0);
}

void ModelChecker::set_score(int score)
{
    if(_last_score < score) {
        _last_score = score;
        cout << "setting score to " << score << endl;
    }
}

void ModelChecker::set_queue(void)
{
    if(option_eq(mcl, state_queue, "bfs"))
        _q = static_cast<StateQueue*>(new BFSQueue);
    else if(option_eq(mcl, state_queue, "dfs"))
        _q = static_cast<StateQueue*>(new DFSQueue);
    else if(STATE_HAS_SCORE)
        _q = static_cast<StateQueue*>(new PrioQueue);
    else
        xi_panic("queue type not supported!");
    assert(_q);
}

void ModelChecker::take_ckpt(State *parent, State* cur)
{
    Checkpoint *ckpt = new Checkpoint;
    assert(ckpt);
    ckpt->len = _test->checkpoint_length();
    ckpt->data = new char[ckpt->len];
    assert(ckpt->data);
    _test->checkpoint(ckpt->data, ckpt->len);
    if(!parent)
        ckpt->parent_id = 0;
    else {
        ckpt->parent_id = parent->start;
        // record path from last checkpoint to this one
        ckpt->trace = parent->trace;
        ckpt->trace.insert (ckpt->trace.end(), 
                            _rand->_choices.begin(),
                            _rand->_choices.end());
    }
    _ckpts[cur->id] = ckpt;
}

State* ModelChecker::new_state(signature_t sig)
{
    State *s;
    int id = num_states();

    if(!STATE_HAS_SCORE)
        s = new State(0, id, sig);
    else
        s = new ScoreState(0, id, sig, _last_score);
    assert(s);

    _visited_states[s->sig] = s->id;
    return s;
}

State* ModelChecker::init_state()
{
    signature_t sig = _test->get_sig();
    State *s = new_state(sig);
    take_ckpt(NULL, s);
	
    return s;
}

void ModelChecker::set_state(State *s)
{
    int oldmode;
    Random *oldr, *newr;
    signature_t sig;

    v_out << "setting state " << s->id << "\n";

    _test->restore(_ckpts[s->start]->data, _ckpts[s->start]->len);

    // set mode saying we're regenerating a state
    oldmode = _mode;
    _mode = MODE_REGEN_STATE;
    run_test(&TestHarness::before_transitions);

    if(s->trace.size() > 0) {
        // get a replay_rand
        oldr = _rand;
        newr = new_random(REPLAY_RAND);
        newr->set_choices(s->trace);
        _rand = newr;
		
        replay();

        // restore _rand
        _rand = oldr;
        delete newr;
    }

    // restore mode
    _mode = oldmode;
    v_out << "done." << endl;
}

State* ModelChecker::get_state(State *parent, int &id)
{
    State *old_s;
    signature_t sig =_test->get_sig();
    if (seen(sig)) {
        id = _visited_states[sig];
        if(!STATE_HAS_SCORE)
            return NULL;

        // TODO: pick the state with a higher score
#if 0
        PrioQueue *q = CAST(PrioQueue*, _q);
        PrioQueue::iterator it;
        for_all_it(it, *q) {

            // is old_s on the queue?
            if((old_s=_q.get(sig)) == NULL)
                return NULL;

            if(_last_score < _scores[old_s->id])
                return NULL;
            else
                ; // TODO replace old_s with s
        }
#endif
    }
	
    State *s = new_state(sig);

    if (_test->should_checkpoint()) {
        take_ckpt(parent, s);
        // truncate the prefix
        s->start = s->id;
    } else {
        s->start = parent->start;
        s->trace = parent->trace;
        s->trace.insert(s->trace.end(), 
                        _rand->_choices.begin(),
                        _rand->_choices.end());
    }
    id = s->id;
    return s;
}
    
bool ModelChecker::seen(signature_t s)
{
    return exists(s, _visited_states);
}

int ModelChecker::run_test(void (TestHarness::*f)(void))
{
    int ret;
    bool can_mutate = (f ==&TestHarness::run_one_transition);
    xi_assert(_last_user_func == NULL,
              "ModelChecker::run_test re-entered!");
    _last_user_func = f;
	
    ret = TEST_EXIT;

#ifdef CONF_USE_EXCEPTION
    try {
        (_test->*f)();
    } catch (CheckerException& r) {
        ret = r.code;;
        status_out << "get checker exception " << r.msg << endl;
    }
#else
    if((ret=setjmp(run_test_env)) == 0)
        f();
#endif
    process_test_return(ret, can_mutate);

    xi_assert(_last_user_func==f, "exiting ModelChecker::run_test "\
              "but _last_user_func is NULL??");
    _last_user_func = NULL;
    return ret;
}

void ModelChecker::check_valid_choose(void)
{
    xi_assert(_last_user_func == &TestHarness::run_one_transition,
              "can only call choose in _test->run_one_transition!");
}

static const char* exit_str(int code)
{
    switch(code) {
    case TEST_EXIT: return "exit";
    case TEST_ABORT: return "abort";
    case TEST_BUG: return "bug";
    }
    assert(0);
}
/*
  this function handles the return code from a user-provided
  function in all model checker modes.  It's ugly.  The good thing
  is that it is the only ugly function.

  MODE_MODEL_CHECK:
  any user provide funtions can return bug, abort.  
  only check new states after_test->test_driver()

  MODE_REGEN_STATE:
  no bug, abort. _test->test_driver() call shouldn't generate a new state

  MODE_REPLAY:
  if get bug or abort, should at the end of the trace.  i.e. no more choices.
  each_test->test_driver() should generate a new state
*/
void ModelChecker::process_test_return(int ret, bool can_mutate)
{
    signature_t sig;
    State *s;
    int id;

    switch(_mode) {
    case MODE_MODEL_CHECK:
        switch (ret) {
        case TEST_BUG:
            status_out << "TRANS: " << _curr->id 
                       << " -> bug [" << _rand->_choices << "]\n";
            store_trace();
            _test->on_error();
            if(++_bugs >= get_option(mcl, max_bugs)) {
                status_out << "max bug limit hit. quit " << endl;
                if(get_option(mcl, clear_on_bug_exit))
                    end_model_check(); // will exit with code 0
                else
                    if(persistent) {
                        // TODO should checkpoint here too
                        Monitor::send(_mon_skt, Monitor::prog_done);
                    }
                exit(0);
            }
            // restart when a bug is found.  this way we don't need to
            // worry about rolling back the changes made before the
            // error is found
            if(persistent)
                restart(1);
            break;
        case TEST_ABORT:
            status_out << "TRANS: " << _curr->id 
                       << " -> abort [" << _rand->_choices << "]\n";
            assert(_mode != MODE_REGEN_STATE);
            // restart explode upon abort.  this way we don't need to
            // worry about rolling back the changes made before the
            // abort
            if(persistent)
                restart(1);
            break;
        case TEST_EXIT:
            if(!can_mutate)
                break;
			
            // check for new states
            s = get_state(_curr, id);
			
            if(s) {
                status_out << "new state " << s->id <<"\n";
                if(get_option(mcl, max_state) > 0
                   && num_states() > get_option(mcl, max_state)) {
                    cout << "max state limit hit. quit " << endl;
                    end_model_check();  // will exit with code 0
                }
                _q->push(s);
				
                /* if(num_states()%get_option(mcl, reboot_states)==0)
                   restart(); */
            }
            status_out << "TRANS: " << _curr->id 
                       << " -> " << id << " [" << _rand->_choices << "]\n";
			
            break;
        case TEST_SKIP:
            // kill this path, do nothing else
            status_out << "TRANS: " << _curr->id 
                       << " -> skip [" << _rand->_choices << "]\n";
            break;
        }
        break;
		
    case MODE_REGEN_STATE:
        xi_assert(ret == TEST_EXIT, "can't get bug or abort or skip "\
                  "when regenerating states!");

        if(can_mutate) {
            sig =_test->get_sig();
            xi_assert(seen(sig), "shouldn't create new states "\
                      "when replay a trace or regenerate a state");
        }
        break;

    case MODE_REPLAY:
        xi_assert(ret != TEST_SKIP, "can't get skip in replay!");
        if(ret == TEST_BUG || ret == TEST_ABORT) {
            status_out << "TRANS: " << _curr->id 
                       << " -> "<< exit_str(ret)
                       << "[" << _rand->_choices << "]\n";
            // TODO: check if there are more choices
        } else if(can_mutate) {
            sig =_test->get_sig();
            xi_assert(!seen(sig), "didn't create new states "\
                      "when replaying a trace fiel");

            s = get_state(_curr, id);
            status_out << "TRANS: " << _curr->id 
                       << " -> " << id << " [" << _rand->_choices << "]\n";
        }
        break;
    }
}

int ModelChecker::run_new_transition(void)
{
    int ret;
    ret = run_test(&TestHarness::before_new_transition);
    if(ret == TEST_EXIT)
        ret = run_test(&TestHarness::run_one_transition);
    if(ret == TEST_EXIT)
        ret = run_test(&TestHarness::after_new_transition);
    return ret;
}

void ModelChecker::model_check (void) 
{
	
    if(persistent)
        connect_to_monitor();

    _test->init();

    if(!_restored) _q->push(init_state());

    while (!_q->empty()) {

        // take a self checkpoint, so we can restart from here
        if(persistent
           && num_states() % get_option(mcl, ckpt_states) == 0)
            self_checkpoint(get_option(mcl, ckpt_file));
        if(output_vvv)
            print(cout, true);

        // if a restart is scheduled, restart now
        if(_need_restart) 
            restart(2);

        // take a state off the state queue
        _curr = _q->pop();

        _rand->reset();
        do {
            // reset unix rand for each transition
            _unix_rand->reset();

            // restore state
            // TODO: should avoid restore unnecessarily
            set_state(_curr);

            // track scores of new transitions only
            _last_score = 0;

            run_new_transition();

            // end of a seq of transitions.  always call this to give the
            // checked system a chance to clean up
            run_test(&TestHarness::after_transitions);

            if(persistent)
                heart_beat();
        } while(_rand->repeat());

#ifdef EAGER_DELETE_STATE
        // should do this to save mem
        // don't do this. now we store all the states in a table
        // need it when outputing error trace
        // clean up the expanded state

        // can do this now, since we store checkpoints
        // TODO: since we can delete states, should use a map rather than a vector for state table
        delete _curr;
        _curr = NULL;
		
#endif
        if(get_option(mcl, simulation))
            break;
    }
    end_model_check();
}

void ModelChecker::replay(void)
{
    do {
        // checking the return of run_test is done in
        // process_test_return
        run_test(&TestHarness::run_one_transition);
    } while(_rand->repeat());
}

void ModelChecker::store_trace(void)
{
    trace_dir();
    store_trace_file();
}

#if 0
void ModelChecker::store_disks(void)
{
    string trace_dir(trace_dir());
    string s;
    s = trace_dir + "/crash.img";
    crash_disk.to_file (s.c_str());
    s = trace_dir + "/fsck-crash.img";
    fsck_crash_disk.to_file(s.c_str());
}
#endif

#define TRACE_DELIM ','
#define TRACE_DELIM_STR ","
#define TRACE_NEW_TRANSITION INT_MAX
void ModelChecker::store_trace_file(void)
{
    string filename(trace_dir());
    filename += "/trace";
    // get the ckpts along the trace
    list<unsigned> ckpt_ids;
    unsigned ckpt_id = _curr->start;
    while (ckpt_id) {
        ckpt_ids.push_front(ckpt_id);
        ckpt_id = _ckpts[ckpt_id]->parent_id;
    }
    ckpt_ids.push_front(0);	// 0 for start state
    /*copy(ckpt_ids.begin(), ckpt_ids.end(),
      ostream_iterator<int>(cout, " "));*/

    ofstream of(filename.c_str());
    xi_assert(of, "can't open trace %s for write", filename.c_str());
    list<unsigned>::iterator it;
    for (it = ckpt_ids.begin(); it != ckpt_ids.end(); it++) {
        const Checkpoint *ckpt = _ckpts[(*it)];
        copy(ckpt->trace.begin(),
             ckpt->trace.end(),
             ostream_iterator<int>(of, TRACE_DELIM_STR));
    }

    // all choices from latest ckpt to current state
    copy(_curr->trace.begin(),
         _curr->trace.end(),
         ostream_iterator<int>(of, TRACE_DELIM_STR));

    // add a separator between the old transitions and the new one
    of<< TRACE_NEW_TRANSITION << TRACE_DELIM;
	
    // all choices from current state to now
    copy(_rand->_choices.begin(),
         _rand->_choices.end(),
         ostream_iterator<int>(of, TRACE_DELIM_STR));

    // store options as well so we can replay this trace
    string s(filename);
    s += ".options";
    print_options(s.c_str());
}

const char* ModelChecker::trace_dir(void)
{
    static char buf[4096];
    sprintf(buf, "%s/%d", get_option(mcl, trace_dir), _bugs);
    mkdir(buf, 0777);
    return buf;
}

void ModelChecker::load_trace_file(const char* filename, trace_t &old_trace, 
                                   trace_t& new_trace)
{
    ifstream inf(filename);
    xi_assert(inf, "can't open trace %s for read", filename);	
    int choice;
    char delim;
    trace_t *trace = &old_trace;

    while(inf) {
        inf >> choice;
        if(!inf)
            break;

        // new transition begins here. switch trace
        if(choice == TRACE_NEW_TRANSITION)
            trace = &new_trace;
        else
            trace->push_back(choice);

        inf >> delim;
        if(!inf)
            break;
        cout << choice << delim << endl;
        xi_assert(delim == TRACE_DELIM, "corrupted trace");
    }
}

void ModelChecker::replay_trace_file(const char* filename)
{
    trace_t old_trace, new_trace;

    load_trace_file(filename, old_trace, new_trace);
    _mode = MODE_REPLAY;

    // switch to replay random
    delete _rand;
    _rand = new_random(REPLAY_RAND);

    // using replay_rand guarentees that there'll be only 1
    // state in the state queue.  so it doesn't matter what
    // queue we use
    _test->init();
    _curr = init_state();
    set_state(_curr); // it will call begin_of_transition() for us;

    if(old_trace.size() > 0) {
        _rand->reset();
        _rand->set_choices(old_trace);
        replay();
    }

    // old trace don't have the notion of new transition.  so
    // new_trace.size() can be zero
    if(new_trace.size() > 0) {
        _rand->reset();
        _rand->set_choices(new_trace);

        run_new_transition();
    }

    run_test(&TestHarness::after_transitions);

    _test->exit();
}

void ModelChecker::print(ostream& o, bool verbose)
{
    State *s;
    int size, i;

    // state queue
    o << *_q;

    // checkpoint table
    Checkpoint *ckpt;
    size = _ckpts.size();
    o << "checkpoint table size " << size << "\n";

    // visited states
    size = num_states();
    o << "total visited states " << size << "\n";
    if(verbose) {
        visited_tbl_t::iterator v_it;
        for_all_iter(v_it, _visited_states)
            o << "(" << (*v_it).first << ", " << (*v_it).second << ") ";
    }
    o << endl;
}

#ifdef DEBUG
static void fd_offset(int fd)
{
    off_t o = lseek(fd, 0, SEEK_CUR);
    vvv_out << "offset " << o << endl;
}
#endif

// this is called only at the place where a new state is about to
// be dequeued
// TODO: may need multiple checkpoints.  now just one
void ModelChecker::self_checkpoint(const char* file)
{
    int fd, size, i;
    State *s;
    char file_tmp[4096];
    static int cnt = 0;

    if(output_vvv)
        print(cout, true);

    strcpy(file_tmp, file);
    strcat(file_tmp, ".tmp");
	
    if((fd=open(file_tmp, O_WRONLY|O_CREAT|O_TRUNC, 0777)) < 0) {
        xi_warn("cannot open the checkpoint file for write!\n");
        return;
    }
		
    // state queue
    _q->save_states_to_file(fd);

    // checkpoint table
    Checkpoint *ckpt;
    size = _ckpts.size();
    write(fd, &size, sizeof size);
    ckpt_tbl_t::iterator ckpt_it;
    for_all_iter(ckpt_it, _ckpts) {
        write(fd, &(*ckpt_it).first,  sizeof (*ckpt_it).first);
        ckpt = (*ckpt_it).second;
        ckpt->to_file(fd);
    }
	
    // visited states
    size = num_states();
    write(fd, &size, sizeof size);
    visited_tbl_t::iterator v_it;
    for_all_iter(v_it, _visited_states) {
        write(fd, &(*v_it).first, sizeof (*v_it).first);
        write(fd, &(*v_it).second, sizeof (*v_it).second);
    }

    // mode
    write(fd, &_mode, sizeof _mode);
	
    // bugs
    write(fd, &_bugs, sizeof _bugs);
	
    // random and unix random?

    fsync(fd);
    close(fd);
	
    // atomic rename
    rename(file_tmp, file);
    cnt ++;
    status_out << "[MC] done checkpointing " << cnt << "\n";
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

ModelChecker* ModelChecker::self_restore(const char* file)
{
    int fd, size, i, id, score;
    State *s;

    ModelChecker *mc = new ModelChecker(MODE_MODEL_CHECK);
    assert(mc);

    if((fd=open(file, O_RDONLY)) < 0) {
        xi_warn("cannot open the checkpoint file for read!\n");
        return mc;
    }

    // state queue
    mc->_q->restore_states_from_file(fd);

    // checkpoint table
    Checkpoint *ckpt;
    read(fd, &size, sizeof size);
    for(i = 0; i < size; i++) {
        read(fd, &id, sizeof id);
        ckpt = new Checkpoint(fd);
        assert(ckpt);
        mc->_ckpts[id] = ckpt;
    }

    // visited states
    read(fd, &size, sizeof size);
    for(i = 0; i < size; i++) {
        signature_t sig;
        read(fd, &sig, sizeof sig);
        read(fd, &id, sizeof id);
        mc->_visited_states[sig] = id;
    }

    // mode
    read(fd, &mc->_mode, sizeof mc->_mode);

    // bugs
    read(fd, &mc->_bugs, sizeof mc->_bugs);
	
    // random and unix random?

    mc->_restored = 1;

    if(output_vvv)
        mc->print(cout, true);

    return mc;
}
