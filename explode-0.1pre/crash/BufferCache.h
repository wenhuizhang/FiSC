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


#ifndef __SECTOR_CACHE_H
#define __SECTOR_CACHE_H

#include "mcl_types.h"
#include "sector_loc.h"
#include "Combination.h"
#include "Powerset.h"
#include "Replayer.h"
#include "Permuter.h"

class BufferPermuter;
struct crash_iterator;


class CacheBase: public Replayer {

public:

    template<class _C, class _E>
    struct iterator;

    virtual void on_error(void) = 0;
    virtual std::ostream& print(std::ostream& o) const = 0;

    virtual void start(void);
    virtual void finish(void);
    virtual void replay(Record* rec);

    void check_crashes(void);

protected:

    virtual void add(SectorRec* s) = 0;
    virtual void remove_or_write(SectorRec* s) = 0;
    virtual void clear(void) = 0;
    virtual crash_iterator* create_iterator(void) = 0;

    CacheBase(BufferPermuter* perm);
    BufferPermuter* _perm;
};

static inline std::ostream& 
operator<<(std::ostream& o, const CacheBase& c)
{  return c.print(o); }

class Cache: public CacheBase {

    typedef Sgi::hash_map<sector_loc_t, 
                          signature_t,
                          hash_sector_loc,
                          eq_sector_loc> cache_t;
public:
    struct iterator; // Powerset

    Cache(BufferPermuter *perm)
        : CacheBase(perm) {}
    
    virtual void on_error(void);
    virtual std::ostream& print(std::ostream& o) const;    

protected:

    virtual void add(SectorRec* s);
    virtual void remove_or_write(SectorRec* s);
    virtual void clear(void);
    virtual crash_iterator* create_iterator(void);

    cache_t _cache;
};



class SectorRec;
class VersionedCache: public CacheBase {

    typedef std::vector<signature_t> versions_t;
    typedef Sgi::hash_map<sector_loc_t,
                          versions_t*,
                          hash_sector_loc,
                          eq_sector_loc> cache_t;
    
    
public:

    struct iterator; // Combination

    VersionedCache(BufferPermuter *perm)
        : CacheBase(perm) {}

    virtual void on_error(void);
    virtual std::ostream& print(std::ostream& o) const;    

protected:

    virtual void add(SectorRec* s);
    virtual void remove_or_write(SectorRec* s);
    virtual void clear(void);
    virtual crash_iterator* create_iterator(void);

    cache_t _cache;
    
};

#endif
