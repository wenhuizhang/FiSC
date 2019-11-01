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


#ifndef __CRASHSIM_H
#define __CRASHSIM_H

/*
  simulates a crash-recovery.
  parses buffer logs
  replay log to generate the set of all possible writes
  whenever this set changes, check subsets  (be aware of redudant checks)
  do internal checks here?  such as the journaling file system check
  why do i need to copy? if I always replay.  need to check this
  how do i know what to check when replaying the log?  application commit record
*/

/*

mutate();
crash(); 
// essentiall stick a crash id to the log, get context, then continue model checking.  not easy to get context
// how about do a fork here. record the id.  
run_check();

but when we replay the log, can't run_check
need to marshal the check(code + data) in a way

how?

simplify it a bit, develop a set of checks

or getcontext?

*/

#include <time.h>

#include <list>
#include <vector>
#include "hash_map_compat.h"

#include "ekm/ekm.h"
#include "rdd/rdd.h"

#include "BlockDev.h"
#include "driver/TestDriver.h"
#include "HostLib.h"
#include "NetLib.h"
#include "RawLog.h"
#include "Log.h"


class Permuter;
class CrashSim: public TestHarness{
private:

    // path name -> component
    Sgi::hash_map<const char*, Component*, eqstr> path_component_hash;
#if 0
    Sgi::hash_map<int, const char*>  dev_id_name_hash;
    Sgi::hash_map<int, BlockDev*> dev_hash;
#endif

    void free_stack(Component *c);

    void threads(void);
    void run_kernel_thread (int pid_idx);
    void check_thread_run(int pid_idx);

#ifdef CONF_EKM_CHOICE
    void pre_choose(int cp);
    void post_choose(void);
#endif

    //void check_crashes(void);

    // these four member variables completely specify a compoent stack
    Component *_stack; // provided by the user
    TestDriver* _driver; // provided by the user
    std::vector<Rdd*> _rdds; // derived from _stack

#if 0
    std::vector<HostLib*> _ekms; // derived from _stack. XXX
                                 // hardwired for now.  first one
                                 // is local, the others are
                                 // remote
#endif
    HostLib* _ekm;
    std::vector<NetLib*> _net_ekms;

    // need to remember checkpointed disks for two purposes
    // restore state.  also, permutation check
    BlockDevDataVec _init_disks;
    // BlockDevDataVec _final_disks;

    threads_t _threads;
    std::vector<int> _thread_timestamps;

    int _should_fail;
    int _debug_choice;

    bool _new_trans_p;
    bool _mounted;
    bool _traced;

    time_t _start_time, _end_time;
    time_t _default_time;

    Permuter *_perm, *_recovery_perm;

public:
    CrashSim();

    virtual void init(void);
    virtual void exit(void);

    virtual void init_state(void);

    virtual void run_one_transition(void);

    virtual signature_t get_sig (void);

    virtual void before_transitions(void);
    virtual void after_transitions(void);
    virtual void before_new_transition(void);
    virtual void after_new_transition(void);
    virtual void on_error(void);

    void mount(void);
    void umount(void);
    void recover(void);

    void mount_user(void);
    void umount_user(void);
    void recover_user(void);

    void mount_kern(void);
    void umount_kern(void);
    void recover_kern(void);

    void start_trace(int want_read=NO_READ);
    RawLogPtr stop_trace(void);

    Permuter *permuter() const {return _perm;}

    TestDriver *driver() const {return _driver;}
    void driver(TestDriver *d) {_driver = d;}

    Component* stack() const {return _stack;}
    void stack(Component *s) {_stack = s;}

    time_t start_time() const {return _start_time;}
    time_t end_time() const {return _end_time;}
    time_t default_time() const {return _default_time;}

    const char* path() const {return _stack->path();}

    BlockDevDataVec& init_disks() {return _init_disks; }

    HostLib* ekm() {return _ekm;}
    NetLib* net_ekm(int i) const {
        assert(0<=i && i<_net_ekms.size());
        return _net_ekms[i];
    }
    
    HostLib* new_ekm(void);
    NetLib* new_net_ekm(const char* host);

    Rdd *new_rdd(void);

    // XXX these two are ad hoc
    // call them before entering into and exit from the
    // kernel.  should really just intercept system calls
    void pre_syscall(void);
    void post_syscall(void);

};

class SimStat
{
public:
    int log_cache_hit;
    int log_cache_miss;
    int logs;
    int recovery_logs;
    int crashes;
    int recovery_crashes;
    int states;
    int queued_states;
    int transitions;
    int driver_mutates;
    int ekm_run;
    int no_ran;
    int ran;
    int kernel_failures;
    int fs_hits;
    int fs_misses;
    int fsck_hits;
    int fsck_misses;
	
    SimStat();
};


std::ostream& operator<<(std::ostream& o, SimStat& st);

void reset_time(char *data, int size, time_t end);
void reset_time(GenericDevData *d, time_t end);

// shortcuts
CrashSim* sim(void);
Permuter* permuter(void);
HostLib* ekm(void);

extern int SIM;
extern int SIM_ANCHOR;
extern int SIM_NEW_TRANS;

extern SimStat sim_stat;
#endif
