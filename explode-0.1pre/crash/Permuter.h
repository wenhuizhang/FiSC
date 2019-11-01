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


#ifndef __PERMUTER_H
#define __PERMUTER_H

#include <memory>

#include "CheckingContext.h"
#include "BufferCache.h"
#include "ReplayMgr.h"
#include "CrashSim.h"

enum commit_epoch_t {
    before_commit = 0,
    in_commit = 2,
    after_commit = 4,
};

CrashSim* sim(void);

class Permuter: public ReplayMgr {

public:

    static void register_record_types(void);
    static rec_cat_t CAT;
    static rec_type_t LAST_CHECK, CUR_CHECK;
    class RecParser;

public:

    class Abort {
    public:
        std::string file;
        int line;
        std::string cause;
        Abort(const char* f, int l, const char* c)
            : file(f), line(l), cause(c) {}
    };

    class checker {
    public:
        // returns CHECK_UNKNOWN, CHECK_FAILED, or CHECK_SUCCEEDED
        virtual int operator() () = 0;
        virtual ~checker() {}
    };

    Permuter(const char* name);
    ~Permuter(void);

    // public crash-checking interface
#include "Permuter-interface.h"
    static void forget_old_crashes(void);
    static void mark_cur_check(void);

    void check_crashes(CheckingContext* ctx, checker* c);

    virtual void on_error(void);

    // accessories called by replayers and checkers
    bool time_is_present(void); // we have already seen CUR_CHECK
                                // durying replay
    bool crash_enabled(void); // should we check for crashes?

    CheckingContext *ctx(void)
    { return _ctx; }

    void next_permuter(Permuter* perm) 
    { _next = perm; }
	
	const char* name()
	{ return _name.c_str(); }

protected:

    class EpochMgr;


    void checker_wants_all(void);
    void checker_wants_now(void);

    virtual void check_crashes(void) = 0;

    CheckingContext *_ctx; // checking context for a check_crashes call
    checker *_checker;     // checker that runs on each simulated crash

    // these two are for error reporting
    BlockDevDataVec *_init_disks;
    RawLogPtr _raw_log;

    EpochMgr *_epoch_mgr; 

    std::string _name; // name of this permuter.  used in error recording

    Permuter *_next;   // next permuter in line.  currently only
                       // have two permuters chained,
};



struct crash_iterator {
    // returns false when all crashes done
    virtual bool next(void) = 0;

    // generate the subset of writes
    virtual void wr_set(wr_set_t& wset) const = 0;

    virtual std::ostream& print(std::ostream& o) const  
    { return o;}
};
typedef std::auto_ptr<crash_iterator> crash_iterator_aptr;

static inline std::ostream& 
operator<<(std::ostream& o, const crash_iterator& i)
{ return i.print(o); }

class CacheBase;

enum cache_type {REGULAR, VERSIONED};

class BufferPermuter: public Permuter {

public:

    BufferPermuter(const char* name, enum cache_type t);
    ~BufferPermuter(void);

    virtual void permute(crash_iterator*);
    virtual void on_error(void);

    BlockDevDataVec& cur_disks(void)
    { return _cur_disks; }
    BlockDevDataVec& crash_disks(void)
    { return _crash_disks; }

protected:

    virtual void check_crashes(void);
    virtual void check_one_crash(crash_iterator*);
    virtual void check_recovery_crashes(void);

    BlockDevDataVec _cur_disks;
    BlockDevDataVec _crash_disks;

    CacheBase *_cache;
};

Permuter* new_permuter(const char* name, const char* type);
#endif
