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


// libmc.a client interface
#ifndef __EXPLODE_H
#define __EXPLODE_H

#include <stdio.h>
#include <assert.h>

#include "sysincl.h"
#include "mcl_types.h"

// internal helper functions for FiSC
#include "util.h"

#ifdef CONF_USE_EXCEPTION
#	if !defined(__cplusplus)
#		error "can't use exception in c code!"
#	endif
class CheckerException;
#else
#	include <setjmp.h>
extern jmp_buf run_test_env;
#endif


// C++ interface 
#ifdef __cplusplus
class TestHarness {
public:
    // Initialize the test harness.  Called once before model checking starts.
    virtual void init(void) {};
    // Clean up the test harness.  Called once when model checking is done.
    virtual void exit(void) {};

    // Create an initial state.  
    virtual void init_state(void) = 0;

    // Do one action on the current state.  Called during model
    // checking, replaying to re-generate a state, or replaying an
    // error trace.
    virtual void run_one_transition(void) = 0;
    
    // Return the signature of a state.
    virtual signature_t get_sig (void) {return 0;}


    // Checkpoint related methods.  
    virtual int should_checkpoint(void) {return 0;};
    virtual int checkpoint_length(void) {return 0;};
    virtual void checkpoint(char* ckpt, int len) {};
    virtual void restore(const char* buf, unsigned len) {init_state();}
    
    // hook methods called during model checking.  From a high
    // level, the eXplode's checking process works as follows: it
    // checkpoints a state by recording the orignial transitions
    // that create the state, and restores the state by replaying
    // the recorded transitions.  After a state is recreated,
    // eXplode will run a new transition that is never run before.  
    // Cannot call random in these hooks.  why????
    virtual void before_transitions(void) {}
    virtual void after_transitions(void) {}
    virtual void before_new_transition(void) {}
    virtual void after_new_transition(void) {}
    // called when eXplode-provided method error() is called
    virtual void on_error(void) {}
};
#endif

// test driver exit status
enum {
    TEST_EXIT = 0,
    TEST_SKIP = -1,
    TEST_ABORT = -2,
    TEST_BUG = -3,
};

/* explode external functions xe_*.  called by the both eXplode and the
   testing program */

#ifdef __cplusplus
extern "C" {
#endif
    typedef int (*choose_fn_t) (int);
    int xe_random (int);
    int unix_random (int);
    int xe_random_fail (void);
    void xe_random_skip_rest(void);

    const char* xe_trace_dir(void);

    int xe_mode(void);
	
    void xe_schedule_restart(void);

    // set the score for current state.  used in heuristic search
    void xe_set_score(int score);

    // explode error reporting functions, called by clients of
    // explode when writing test drivers, storage components.
    // these will terminiate the current exeuction of the checked
    // system
    void error_ (const char*fn, int line,
                 const char* type, const char *fmt, ...);
#define error(type, fmt, arg...) \
	error_(__FILE__, __LINE__, type, fmt, ##arg)

    void xe_msg(int code, const char* fn, int line,
                     const char *fmt, ...);
#define xe_abort(fmt, arg...) \
	xe_msg(TEST_ABORT, __FILE__, __LINE__, fmt, ##arg)
#define xe_skip(fmt, arg...) \
	xe_msg(TEST_SKIP, __FILE__, __LINE__, fmt, ##arg)


    // process command line arguments and create the model checker
    void xe_process_cmd_args(int argc, char *argv[]);

    void xe_check(void *test_harness);


    // explode internal assertion functions  xi_*
    // have the if statement in macro so we can use anything valid as @cond
#define xi_assert(cond, fmt, arg...) \
    do {\
	if(!(cond)) \
	    xi_assert_failed(__FILE__, __LINE__, fmt, ##arg); \
    } while(0)

#define xi_panic(fmt, arg...) \
    xi_assert_failed(__FILE__, __LINE__, fmt, ##arg)

#define xi_warn(fmt, arg...) \
    xi_warn_(__FILE__, __LINE__, fmt, ##arg)

    void xi_warn_(const char* fn, int line, 
                       const char* fmt, ...);
    __attribute__((noreturn))
        void xi_assert_failed(const char* fn, int line, 
                              const char* fmt, ...);

#ifdef __cplusplus
}
#endif




/* debug */
#define DEBUG_NON_REENTRANCE
#ifdef DEBUG_NON_REENTRANCE
#define DECLARE_NON_REENTRANT \
static int ___entries = 0; \
xi_assert(___entries==0, "re-entering non-reentrant function!"); \
___entries++

#define RETURN_NON_REENTRANT(x) ___entries--; return x
#else
#define DECLARE_NON_REENTRANT
#define RETURN_NON_REENTRANT
#endif

/* options */
#include "mcl-options.h"

#endif
