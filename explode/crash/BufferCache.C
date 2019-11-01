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


#include <boost/scoped_ptr.hpp>

#include "ekm/ekm_log.h"
#include "BufferCache.h"
#include "Permuter.h"
#include "crash-options.h"

using namespace std;

////////////////////////////////////////////////////////
//               CacheBase                         

template<class _C, class _E>
struct CacheBase::iterator: public crash_iterator
{
public:
    iterator(_C* o): _o(o),
                     _e(get_option(permuter, kernel_writes),
                        get_option(fsck, deterministic_threshold), 
                        get_option(fsck, random_tries))
    { }

    virtual bool next(void)
    { return _e.next(); }

    virtual ostream& print(ostream& o) const {
        wr_set_t wset;
        wr_set_t::iterator it;
        
        wr_set(wset);
        o << _e.iteration() << ": subset of writes:\n";// << *_e <<"\n";
        for_all_iter(it, wset)
            o << (*it).first << ":" << (*it).second << "\n";
        return o;
    }

protected:
    _E _e; // enumerates possible subsets
    _C* _o; // owner of this iterator
};

CacheBase::CacheBase(BufferPermuter* perm)
    : _perm(perm)
{
    _interests.push_back(EKM_SECTOR_CAT);
}

void CacheBase::start(void)
{
    clear();
}

void CacheBase::replay(Record *rec)
{
    vvv_out << "Cache: replaying " 
            << LogMgr::the()->name(rec) << endl;
    SectorRec *s = (SectorRec*)(rec);

    if(s->type() == EKM_ADD_SECTOR)
        add(s);
    else if(s->type() == EKM_REMOVE_SECTOR
            || s->type() == EKM_WRITE_SECTOR) 
        remove_or_write(s);
}

void CacheBase::finish(void)
{
    // cache might still have buffer writes.  check crashes
    check_crashes();
    clear();
}

void CacheBase::check_crashes(void)
{
    if(_perm->crash_enabled()) {
        vvv_out << "CACHE: " << *this << endl;
        // generate the last batch of crashes
        crash_iterator_aptr i 
            = crash_iterator_aptr(create_iterator());
        _perm->permute(i.get());
    }
}

////////////////////////////////////////////////////////
//                 Cache

struct Cache::iterator
    : public CacheBase::iterator<Cache, Powerset>
{
public:
    typedef CacheBase::iterator<Cache, Powerset> _Parent;
    
    iterator(Cache* o): _Parent(o)
    { _e.init(_o->_cache.size()); }

    virtual void wr_set(wr_set_t& wset) const {
        int i = 0;
        const Powerset::set_t& subset = *_e;
        cache_t::iterator it;
        
        for_all_iter(it, _o->_cache) {
            if(!exists(i, subset)) {
                i++;
                continue;
            }
            const sector_loc_t &loc = (*it).first;
            wset[loc] = _o->_cache[loc];
            i++;
        }
    }
};

void Cache::on_error(void)
{
    // TODO
}

ostream& Cache::print(ostream& o) const
{
    cache_t::const_iterator i;
    if(_cache.size() == 0)
        return o << "empty";
    
    o << "size=" << _cache.size() << "\n";
    for_all_iter(i, _cache)
        o<<(*i).first<<":"<<(*i).second<<"\n";
    return o;
}

void Cache::add(SectorRec* s)
{
    // if the write is identical to what's on-disk, skip it.
    if(_perm->cur_disks().have_sector(s))
        return;

    _cache[s->loc()] = s->sig();
}

void Cache::remove_or_write(SectorRec* s)
{
    // This can happen because:
    // (1) some mark_clean or end_request in the ekm log are dangling.
    // (2) when adding this sector, its value is already in cache
    //     or on-disk.  so we didn't add.
    if(!exists(s->loc(), _cache)) {
        if(s->type() == EKM_WRITE_SECTOR)
            _perm->cur_disks().write(s);
        return;
    }

    // Check for crashes when the set of potential disk writes
    // shrinks, which trivially avoids checking redundant crash-
    // disks
    // FIXME: there is a problem with only generating crashes when
    // writeset shrinks.  should also consider the epoches because
    // we should not carry crashes over them.
    check_crashes();

    // remove sector from cache
    _cache.erase(s->loc());
    if(s->type() == EKM_WRITE_SECTOR)
        _perm->cur_disks().write(s);
}

void Cache::clear(void)
{
    _cache.clear();
}

crash_iterator* Cache::create_iterator(void)
{
    return new iterator(this);
}

/////////////////////////////////////////////////////////
//             Versioned Cache


struct VersionedCache::iterator
    : public CacheBase::iterator<VersionedCache, Combination>
{
public:
    
    typedef CacheBase::iterator<VersionedCache, Combination> _Parent;

    iterator(VersionedCache* o) : _Parent(o)
    { init_e(); }

    virtual void wr_set(wr_set_t& wset) const {
        int i = 0;
        const Combination::set_t& subset = *_e;
        cache_t::iterator it;
        
        for_all_iter(it, _o->_cache) {
            if(subset[i] == 0) {
                i++;
                continue;
            }
            const sector_loc_t &loc = (*it).first;
            wset[loc] = (*_o->_cache[loc])[subset[i]-1];
            i++;
        }
    }
    
protected:

    void init_e(void) {
        // set ceilings for _e
        Combination::set_t counts;
        cache_t::iterator it;
        for_all_iter(it, _o->_cache) {
            versions_t *w;
            w = (*it).second;
            counts.push_back(w->size());
        }
        _e.ceilings(counts);
    }
};

void VersionedCache::on_error(void)
{
    // TODO: write cache status to disk
}


ostream& VersionedCache::print(ostream& o) const
{
    cache_t::const_iterator i;
    if(_cache.size() == 0)
        return o << "empty";
    
    o << "size=" << _cache.size() << "\n";
    for_all_iter(i, _cache) {
        o<<(*i).first<<":";
        versions_t *v = (*i).second;
        copy(v->begin(), v->end(), ostream_iterator<signature_t>(o,","));
        o<<"\n";
    }
    return o;
}

void VersionedCache::add(SectorRec* s)
{
    versions_t *v;

    // if the write is identical to what's on-disk, skip it.
    if(_perm->cur_disks().have_sector(s))
        return;

    if(!exists(s->loc(), _cache)) {
        // no such sector in cache. 

        // add this sector to cache
        v = new versions_t; assert(v);
        _cache[s->loc()] = v;
        v->push_back(s->sig());
        return;
    }
    
    // sector already in cache.  check if the cache has this
    // version.

    v = _cache[s->loc()];
    assert(v->size() > 0);

    // identical to the latest version, do nothing
    if(s->sig() == v->back())
        return;

    // if any previous version (at most one) is the identical,
    // remove it
    versions_t::iterator it;
    for(it=v->begin();it!=v->end();it++)
        if(s->sig() == (*it)) {
            v->erase(it);
            break;
        }
    v->push_back(s->sig());
}

void VersionedCache::remove_or_write(SectorRec* s)
{
    // This can happen because:
    // (1) some mark_clean or end_request in the ekm log are dangling.
    // (2) when adding this sector, its value is already in cache
    //     or on-disk.  so we didn't add.
    if(!exists(s->loc(), _cache)) {
        if(s->type() == EKM_WRITE_SECTOR)
            _perm->cur_disks().write(s);
        return;
    }

    // Check for crashes when the set of potential disk writes
    // shrinks, which trivially avoids checking redundant crash-
    // disks
    // FIXME: there is a problem with only generating crashes when
    // writeset shrinks.  should also consider the epoches because
    // we should not carry crashes over them.
    check_crashes();

    // remove sector from cache
    _cache.erase(s->loc());
    if(s->type() == EKM_WRITE_SECTOR)
        _perm->cur_disks().write(s);
}

void VersionedCache::clear(void)
{
    cache_t::iterator it;
    for_all_iter(it, _cache)
        delete (*it).second;
    _cache.clear();
}

crash_iterator* VersionedCache::create_iterator(void)
{
    return new iterator(this);
}
