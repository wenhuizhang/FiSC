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


#include "Permuter.h"
#include "SyscallPermuter.h"
#include "CheckingContext.h"
#include "Log.h"
#include "CrashSim.h"
#include "BufferCache.h"

using namespace std;

rec_cat_t Permuter::CAT = EKM_INVALID_CAT;
rec_type_t Permuter::LAST_CHECK, Permuter::CUR_CHECK;

class Permuter::RecParser: public DefaultParser<AppRec>
{
public:
    // remove extra LAST_CHECK records
    void finish(Log *l) {

        // only keep the last LAST_CHECK if there are more than
        // one LAST_CHECK.  If there is none, create a LAST_CHECK
        // and put it at the beginning
        Log::iterator it, last, prev;

        for_all_iter(last, *l)
            if((*last)->type() == LAST_CHECK)
                break;

        // no LAST_CHECK in current log, push one in the front
        if(last == l->end())
            l->push_front(new AppRec(LAST_CHECK));
        else 
            // remove extra LAST_CHECK records
            for(it=l->begin(); it!=last; it=prev, ++it) {
                prev = it;
                if((*it)->type() == LAST_CHECK) {
                    delete *it;
                    l->erase(it);
                }
            }
    }
};

void Permuter::register_record_types(void)
{
    if (CAT != EKM_INVALID_CAT) 
        return;
    CAT = LogMgr::the()->new_category("perm", new RecParser);
    LAST_CHECK = LogMgr::the()->new_type(CAT, "perm_last_check");
    CUR_CHECK = LogMgr::the()->new_type(CAT, "perm_cur_check");
}


class Permuter::EpochMgr: public Replayer {

private:

    enum epoch_t {
        LONG_GONE_PAST = 0,
        RECENT_PAST = 1,
        NOW = 2,
    };

public:

    EpochMgr()
    { 
        _interests.push_back(Permuter::CAT);
        _start_checking = RECENT_PAST; // by default check all crashes
        _cur = LONG_GONE_PAST;
    }

    virtual void start(void)
    { _cur = LONG_GONE_PAST; }

    virtual void replay(Record* rec)
    {
        if(rec->type() == Permuter::LAST_CHECK)
            _cur = RECENT_PAST;
        else if(rec->type() == Permuter::CUR_CHECK)
            _cur = NOW;
    }

    bool time_is_present(void) 
    { return _cur == NOW; }
    
    bool crash_enabled(void)
    { return _cur >= _start_checking; }

    void checker_wants_all(void)
    { _start_checking = RECENT_PAST; }

    void checker_wants_now()
    { _start_checking = NOW; }

private:

    epoch_t _cur;
    epoch_t _start_checking;
};




Permuter::Permuter(const char* name)
    :_next(NULL), _name(name)
{
    register_record_types();

    _epoch_mgr = new EpochMgr;
    register_replayer(_epoch_mgr);

}

Permuter::~Permuter(void)
{
    delete _epoch_mgr;
    if(_next)
        delete _next;
}

void Permuter::check_crashes(CheckingContext* ctx, checker* c)
{
    _ctx = ctx; 
    _checker = c;

    check_crashes();
    
    _ctx = NULL;
    _checker = NULL;
}

void Permuter::on_error(void)
{
    // write ekm log
    static char name[4096];
    sprintf(name, "%s/ekm-log", xe_trace_dir());
    _raw_log->to_file(name);

    // write init_disks
    _init_disks->to_file(xe_trace_dir(), "init");

    if(_next)
        _next->on_error();
}

void Permuter::mark_cur_check(void)
{
    sim()->ekm()->ekm_app_record(CUR_CHECK, 0, NULL);
}

void Permuter::forget_old_crashes(void)
{
    sim()->ekm()->ekm_app_record(LAST_CHECK, 0, NULL);
}

bool Permuter::time_is_present(void)
{
    return _epoch_mgr->time_is_present();
}

bool Permuter::crash_enabled(void)
{
    return _epoch_mgr->crash_enabled();
}

void Permuter::checker_wants_all(void)
{
    _epoch_mgr->checker_wants_all();
}

void Permuter::checker_wants_now(void)
{
    _epoch_mgr->checker_wants_now();
}

static CacheBase* new_cache(enum cache_type type, BufferPermuter *perm)
{
    switch(type) {
    case REGULAR:
        return new Cache(perm);
    case VERSIONED:
        return new VersionedCache(perm);
    default:
        xi_panic("unknown cache type %d!", type);
    }
}

BufferPermuter::BufferPermuter(const char* name, enum cache_type t)
    : Permuter(name)
{
    _cache = new_cache(t, this);
    register_replayer(_cache);
}

BufferPermuter::~BufferPermuter(void)
{
    delete _cache;
}

void BufferPermuter::on_error(void)
{
    // TODO: write more info.  also, preppend _name to the prefix.
    _crash_disks.to_file(xe_trace_dir(), "crash");
    _cur_disks.to_file(xe_trace_dir(), "current");

    _cache->on_error();

    Permuter::on_error();
}

void BufferPermuter::permute(crash_iterator *i)
{
    static int ncrash = 0;
    v_out << "========== " << name() << ": CHECKING CRASHES "
		  << ncrash++ <<"=====================\n"
          << "CACHE: " << *_cache;

    while(i->next())
        check_one_crash(i);
}

void BufferPermuter::check_crashes(void)
{
    _init_disks = &_ctx->init_disks();
    _raw_log = _ctx->_raw_log;
    _cur_disks = _ctx->init_disks();
    replay(_ctx->log());
}

#include "storage/Fs.h"

void BufferPermuter::check_one_crash(crash_iterator *i)
{
    wr_set_t wset;

    v_out << "-----------------------------------------\n"
          << "CRASH: " << *i << endl;

    i->wr_set(wset);

    // Generate crash-disks, and copy them to RDD
    _crash_disks = _cur_disks;
    _crash_disks.write_set(wset);
    _crash_disks.set_dev_data();

    sim()->recover();
    // call user's checker to check that the recovered storage
    // system is valid
    (*_checker)();
    sim()->umount();

    if(_next != NULL)
        check_recovery_crashes();
}

void BufferPermuter::check_recovery_crashes(void)
{
    // Copy crash-disks to RDD, and trace recovery.
    _crash_disks.set_dev_data();

    sim()->start_trace();
    sim()->recover();

    // This is the context for recovery crash checking
    CheckingContext ctx(_crash_disks);

    // We can re-use the same _checker: the stable state should
    // not change regardless how many times we crash recovery.
    _next->check_crashes(&ctx, _checker);
}

Permuter* new_permuter(const char* name, const char* type)
{
    if(strcasecmp(type, "versioned_buffer") == 0)
        return new BufferPermuter(name, VERSIONED);
    else if(strcasecmp(type, "regular_buffer") == 0)
        return new BufferPermuter(name, REGULAR);
    else if(strcasecmp(type, "syscall") == 0)
        return new SyscallPermuter(name);
    else
        xi_panic("Permuter type %s not supported!", type);
}
