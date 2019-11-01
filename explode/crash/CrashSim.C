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


#include <sys/ioctl.h>
#include <errno.h>

#include <iostream>
#include <algorithm>
#include <iterator>

#include "mcl.h"
#include "CrashSim.h"
#include "Permuter.h"
#include "ekm/ekm.h"
#include "ekm_lib.h"
#include "Replayer.h"
#include "ModelChecker.h"
#include "ekm/ekm_log.h"
#include "NetLib.h"
#include "hash_map_compat.h"
#include "crash-options.h"

using namespace std;

SimStat sim_stat;

CrashSim::CrashSim(void)
{
    _stack = NULL;
    _driver = NULL;
    _ekm = NULL;
    _perm = NULL;
    _recovery_perm = NULL;
    _mounted = false;
    _traced = false;
}

void CrashSim::threads(void) 
{
    int i;

    // don't run kblockd
    int kblockd = Component::get_pid("kblockd/0");
    _threads.clear();
    _threads.push_back(kblockd);
    _stack->threads_all(_threads);
	
    for_all(i, _threads)
        _ekm->ekm_no_run(_threads[i]);

    // set thread timestamps so we can check if they actually run
    // or not
    _thread_timestamps.clear();
    for_all(i, _threads)
        _thread_timestamps.push_back(_ekm->ekm_ran(_threads[i]));

#if 0
    v_out << "threads " << _threads << endl
          << "thread timestamps " << _thread_timestamps << endl;
#endif
}

void CrashSim::run_kernel_thread (int pid_idx)
{
    v_out << "running kernel thread "<< pid_idx << "...";

    assert(pid_idx >= 0 && pid_idx < (int)_threads.size());
    pre_syscall();
    if(_ekm->ekm_run(_threads[pid_idx]) < 0) {
        perror("ekm_run");
        xi_panic("can't invoke ekm_run");
    }
    post_syscall();

    v_out << "done\n";
}

void CrashSim::check_thread_run(int pid_idx)
{
    int i;
    int new_timestamp;
    for_all(i, _threads) {

        new_timestamp = _ekm->ekm_ran(_threads[i]);

        // print a warning when a thread 
        // 1) was scheduled to run, but didn't,
        // 2) or wasn't scheduled to run, but did
        if(i == pid_idx && new_timestamp == _thread_timestamps[i]) {
            xi_warn("thread %d not run!", _threads[i]);
            sim_stat.no_ran ++;
        }
        else if (i != pid_idx && new_timestamp != _thread_timestamps[i]) {
            xi_warn("thread %d ran!", _threads[i]);
            sim_stat.ran ++;
        }
    }
}

void CrashSim::pre_syscall(void)
{
    if(get_option(ekm, choice_ioctl)) {
#ifdef CONF_EKM_CHOICE
        pre_choose(0);
#else
        xi_panic("no kernel choice.  kernel module doesn't support it!");
#endif
    }
}

void CrashSim::post_syscall(void)
{
    if(get_option(ekm, choice_ioctl)) {
#ifdef CONF_EKM_CHOICE
        post_choose();
#else
        xi_panic("no kernel choice.  kernel module doesn't support it!");
#endif
    }
}


#if 0
void CrashSim::post_transitions(void)
{
    umount();
    // FIXME umount the driver after we umount fs, so we can catch umount bugs
    _driver->umount();

    _new_trans_p = false;

    if(get_option(ekm, disk_choice_ioctl)) {
#ifdef CONF_EKM_CHOICE
        post_choose();
#else
        xi_panic("no kernel choice.  kernel module doesn't support it!");
#endif

        _init_disks[0]->revive();
    }
}
#endif

#ifdef CONF_EKM_CHOICE
void CrashSim::pre_choose(int cp)
{
    _should_fail = xe_random(2);
    if(!_should_fail) return;

    if(_debug_choice != 0)
        xi_panic("pre_syscall re-entrant!");
    _debug_choice = 1;

    struct ekm_choice choice;

    memset(&choice, 0, sizeof choice);
    choice.pass_count = 
        xe_random(get_option(ekm, max_kernel_choicepoints));
    choice.possibility = 1;
    choice.choice = cp;
    choice.extra_failures = get_option(ekm, extra_kernel_failures);
	
    xi_assert(_threads.size()<sizeof(choice.pid)/sizeof(choice.pid[0]),
              " too many threads!\n");
    int i;
    choice.pid[0] = getpid(); // for ourself
    for(i = 0; i < (int)_threads.size(); i++)
        choice.pid[i+1] = _threads[i];

    cout<<"failing " << choice.pass_count << endl;
    if(_ekm->ekm_set_choice(&choice) < 0) {
        perror("ekm_set_choice");
        xi_panic("can't invoke ekm_set_choice");		
    }
}

void CrashSim::post_choose(void)
{
    if(!_should_fail) return;

    if(_debug_choice != 1)
        xi_panic("post_syscall re-entrant!");
    _debug_choice = 0;

    struct ekm_choice choice;

    if(_ekm->ekm_get_choice(&choice) < 0) {
        perror("ekm_get_choice");
        xi_panic("can't invoke ekm_get_choice");
    }

    // if nothing failed.  skip the rest choices of the last choice point
    printf("pass count is %d\n", choice.pass_count);
    if(choice.pass_count >= 0)
        xe_random_skip_rest();
	
    if(choice.pass_count < 0) {
        sim_stat.kernel_failures ++;
        cout<<sim_stat<<endl;
    }
}
#endif

void CrashSim::free_stack(Component *c)
{
    component_vec_t::iterator it;
    for_all_iter(it, c->children)
        free_stack(*it);

    delete c;
}

void CrashSim::init(void)
{
    _should_fail = 0;
    _debug_choice = 0;

    // sanity checks
    xi_assert(!get_option(ekm, choice_ioctl)
              || !get_option(ekm, disk_choice_ioctl),
              "ekm::choice_ioctl and ekm::disk_choice_ioctl "\
              "can't be both set");
    xi_assert(_ekm, "no EKM created\n");
    xi_assert(_stack, "no storage stack created\n");
    
    // initialize permuter
    _perm = new_permuter("permuter", get_option(permuter, type));
    if(option_ne(recovery_permuter, type, "none")) {
        Permuter *next_perm = new_permuter("recovery_permuter", 
                                           option(recovery_permuter, type));
        _perm->next_permuter(next_perm);
    }

    if(_driver->get_replayer())
        _perm->register_replayer(_driver->get_replayer());

    // load kernel modules
    _ekm->load_ekm();
    xi_assert(_rdds.size(), "no RDD created\n");
    // TODO: check all rdd have the same sector size
    _ekm->load_rdd(_rdds.size(), _rdds[0]->sectors());

    // clear garbage in ekm log.  just in case.  XXX why only
    // _ekm (and not other ekms, e.g. the vmware remote ekm)?
    _ekm->ekm_empty_log();

    // record starting time.  used to reset time-dependent data
    time(&_start_time);
    // set _default_time.  if 0, use _start_time
    _default_time = get_option(simulator, default_time);

    if(_default_time == 0)
        _default_time = _start_time;

    // initialize the real storage stack
    _stack->init_all();
    _mounted = true;

    sync();
    umount();

    // get and save initial rdd disks, so that later we can use
    // them to restore the initial state.  also reset all time
    // values to _default_time
    _init_disks.get_dev_data<Rdd>(_rdds);
    int i;
    for_all(i, _rdds)
        reset_time(_init_disks[i], time(NULL));
}

void CrashSim::exit()
{
    umount();
    free_stack(_stack);

    if(_perm)
        delete _perm;

    _init_disks.clear();

    // unload rdd module
    _ekm->unload_rdd();

    if(_driver)
        delete _driver;

}

void CrashSim::init_state()
{
    int i;
    // restore rdd to initial state
    _init_disks.set_dev_data();
}

void CrashSim::run_one_transition()
{
    int c, choices;
#if 0
 again:
    choices = 0;
    if(get_option(ekm, run_ioctl))
        choices += _threads.size();

    choices += 1; // +1 for the test driver thread
    c = xe_random(choices);

    if(c == 0)
        _driver->mutate();
    else
        run_kernel_thread(c-1);
	
    check_thread_run(c-1);

    if(get_option(mcl, simulation)) {
        static int i = 1;
        if(i++  < get_option(mcl, simulation_length))
            goto again;
        // replay will call end_of_transition, no need to check crashes here
        // check_crashes();
    }
#else
    int length = get_option(mcl, simulation_length);
    if(_new_trans_p)
        sim_stat.transitions ++;
    while(length -- > 0) {

        choices = 0;
        if(get_option(ekm, run_ioctl))
            choices += _threads.size();
		
        choices += 1; // +1 for the test driver thread
        c = xe_random(choices);
		
        if(c == 0) {
            _driver->mutate();
            if(_new_trans_p)
                sim_stat.driver_mutates++;
        }
        else {
            run_kernel_thread(c-1);
            if(_new_trans_p)
                sim_stat.ekm_run ++;
        }
		
        check_thread_run(c-1);
    }
#endif
}

signature_t CrashSim::get_sig (void)
{
    return _driver->get_sig();
}

void CrashSim::before_transitions(void)
{
    _ekm->ekm_empty_log(); // just in case

    _new_trans_p = false;

    if(get_option(ekm, disk_choice_ioctl))
        _rdds[0]->kill();

    start_trace();

    mount();
    _driver->mount();
}

void CrashSim::before_new_transition(void)
{
    _new_trans_p = true;

    if(get_option(ekm, disk_choice_ioctl)) {
#ifdef CONF_EKM_CHOICE
        pre_choose(EKM_DISK_READ | EKM_DISK_WRITE);
#else
        xi_panic("no kernel choice.  kernel module doesn't support it!");
#endif
    }
}

void CrashSim::after_new_transition(void)
{
    _new_trans_p = false;
    
    if(get_option(ekm, disk_choice_ioctl)) {
#ifdef CONF_EKM_CHOICE
        post_choose();
#else
        xi_panic("no kernel choice.  kernel module doesn't support it!");
#endif

        _rdds[0]->revive();
    }
}

void CrashSim::after_transitions(void)
{
    // just in case
    umount();
    // FIXME umount the driver after we umount fs, so we can catch umount bugs
    _driver->umount();
}

void CrashSim::on_error(void)
{
    _perm->on_error();
}

void CrashSim::mount(void)
{
    if(_mounted) return;

    _stack->mount_all();

    // initialize threads
    threads();

    _mounted = true;
}

void CrashSim::umount(void)
{
    if(!_mounted) return;
    
    _stack->umount_all();

    _mounted = false;
}

void CrashSim::recover(void)
{
    _stack->recover_all();

    _mounted = true;
}

void CrashSim::mount_user(void)
{
    _stack->mount_user();
}

void CrashSim::umount_user(void)
{
    _stack->umount_user();

    // stack is fully mounted
    _mounted = true;
}

void CrashSim::recover_user(void)
{
    _stack->recover_user();

    // stack is fully mounted
    _mounted = true;
}

void CrashSim::mount_kern(void)
{
    _stack->mount_kern();
}

void CrashSim::umount_kern(void)
{
    _stack->umount_kern();

    // stack is fully umounted
    _mounted = false;
}

void CrashSim::recover_kern(void)
{
    _stack->recover_kern();
}

void CrashSim::start_trace(int want_read)
{
    int i;

    if(_traced) {
        xi_warn("can't start trace: already tracing devices");
        return;
    }

    for_all(i, _rdds)
        _rdds[i]->start_trace(want_read);

    _traced = true;
}

boost::shared_ptr<RawLog> CrashSim::stop_trace()
{
    int i;

    if(!_traced) {
        xi_warn("can't stop trace: already stopped tracing devices");
        return boost::shared_ptr<RawLog>(new RawLog);
    }

    for_all(i, _rdds)
        _rdds[i]->stop_trace();

    _traced = false;
    return boost::shared_ptr<RawLog>(new RawLog);
}

HostLib* CrashSim::new_ekm(void)
{
    assert(_ekm == NULL);
    _ekm = new HostLib; assert(_ekm);
    return _ekm;
}

NetLib* CrashSim::new_net_ekm(const char* host)
{
    // TODO: check duplicates
    NetLib* net_ekm = new NetLib(host); assert(net_ekm);
    _net_ekms.push_back(net_ekm);
    return net_ekm;
}

// FIXME: only creates local disks.  worry about remote rdd later
Rdd* CrashSim::new_rdd(void)
{
    char path[256];
    sprintf(path, "/dev/rdd%d", _rdds.size());
    Rdd* d = new Rdd(path, _ekm); assert(d);
    d->sectors(get_option(rdd, sectors));
    _rdds.push_back(d);
    return d;
}

SimStat::SimStat(void) 
{
    log_cache_hit       = 0;
    log_cache_miss      = 0;
    logs                = 0;
    recovery_logs       = 0;
    crashes             = 0;
    recovery_crashes	= 0;
    states              = 0;
    queued_states       = 0;
    transitions         = 0;
    driver_mutates      = 0;
    ekm_run             = 0;
    no_ran              = 0;
    ran                 = 0;
    kernel_failures     = 0;
    fs_hits             = 0;
    fs_misses           = 0;
    fsck_hits           = 0;
    fsck_misses         = 0;
}

ostream& operator<<(ostream& o, SimStat& st)
{
    time_t t;
    time(&t);
    st.states = explode->num_states();
    st.queued_states = explode->num_states_in_queue();
    o << "SimStat: "
      << "[time="              << t                     <<"]"
      << "[log_cache_hit="     << st.log_cache_hit	<<"] "
      << "[log_cache_miss="    << st.log_cache_miss	<<"] "
      << "[logs="	       << st.logs	        <<"] "
      << "[recovery_logs="     << st.recovery_logs	<<"] "
      << "[crashes="           << st.crashes            <<"] "
      << "[recovery_crashes="  << st.recovery_crashes   <<"] "
      << "[states="            << st.states             <<"] "
      << "[queue="             << st.queued_states      <<"] "
      << "[transitions="       << st.transitions        <<"] "
      << "[driver_mutates="    << st.driver_mutates     <<"] "
      << "[ekm_run="           << st.ekm_run            <<"] "
      << "[no_ran="            << st.no_ran             <<"] "
      << "[ran="               << st.ran                <<"] "
      << "[kernel_failures="   << st.kernel_failures    <<"] "
      << "[fs_hits="           << st.fs_hits            <<"] "
      << "[fs_misses="         << st.fs_misses          <<"] "
      << "[fsck_hits="         << st.fsck_hits          <<"] "
      << "[fsck_misses="       << st.fsck_misses        <<"] "
        ;
    return o;
}

// TODO: these should be in a class
// need this, because we want to have a sector db
void reset_time(char* data, int size, time_t end)
{
    time_t start = sim()->start_time();
    time_t deflt = sim()->default_time();
    time_t t;
    char *p = data, *e = data + size;
	
    while(p < e) {
        memcpy(&t, p, sizeof t);

        if(t >= start && t <= end && t != deflt) {
            memcpy(p, &deflt, sizeof deflt);
#if 0
            vvv_out << "reset time " 
                    << t << " to " 
                    << def << endl;
#endif
        }
		
        p += sizeof t;
    }

}

// need to reset disks, because we'll check that init disks + log
// = final disks, and we'll use hash to read sectors from db.  XXX
// actually, do we really need this???  can just loose the
// comparison function
void reset_time(GenericDevData *d, time_t end)
{
    reset_time(d->data(), d->size(), end);
}

