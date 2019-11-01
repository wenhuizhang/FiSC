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
   This extremly-ugly code parses the Linux buffer records
   (LinuxBlockRec) into sector records (SectorRec).  
   
   The main reason that it is so ugly is, we log Linux buffer
   cache operations and then reverse engineer these operations to
   find out when a buffer is added to or removed from the
   potential write set.  The Linux buffer cache operations do not
   map directly to the idealized operations on the potential write
   set.  Specifically, EKM_BH_clear doesn't always map to
   EKM_REMOVE_BLOCK.  

   Here is one examplee in which mark_buffer_clear doesn't
   translate to removing buffers from the potential write set
   because the buffer is submitted for disk write.

   mark_buffer_dirty();
   lock();
   submit();
   mark_buffer_clear(); // can't clear. since buffer is cleared
   unlock();
   end_request();

   Here is another example.  Since the buffer is locked when
   mark_buffer_clear is called, we can't translate it to removing
   buffers from the potential write set.  The buffer may or may
   not be submitted for disk write, so we have to defer the
   translation of mark_buffer_clear to EKM_REMOVE_BLOCK to unlock
   time.

   mark_buffer_dirty();
   lock();
   mark_buffer_clear(); // can't clear now. wait until unlock
   submit();
   unlock();
   end_request();

   or

   mark_buffer_dirty();
   lock();
   mark_buffer_clear(); // can't clear now. wait until unlock
   unlock();
   
   We also do some checking in this file. For example, we can
   check that lock is always paired with a subsequent unlock.
   
*/

#ifndef __LINUX_LOG_H
#define __LINUX_LOG_H

#include <boost/shared_ptr.hpp>
#include "hash_map_compat.h"

#include "sector_loc.h"
#include "options.h"
#include "Log.h"
#include "ekm/ekm_log.h"
#include "ChunkDB.h"
#include "../storage/storage-options.h"

class LinuxBlockRec: public BlockRec {

public:
    LinuxBlockRec(rec_type_t t, const char* &off)
        : BlockRec(t, off, EKM_REC_TYPE_HAS_DATA(t)) 
    {}
};


class LinuxBlockRecParser: public RecParser {

    struct sector_state;
    typedef Sgi::hash_map<sector_loc_t,
                          sector_state*,
                          hash_sector_loc,
                          eq_sector_loc> states_t;
    typedef boost::shared_ptr<LinuxBlockRec> LinuxBlockRecPtr;

    struct sector_state {
        bool locked;
        bool dirtied;
        bool delayed_clear;
        bool submitted;

        LinuxBlockRecPtr delayed_clear_rec;
        SectorRec *prev_dirty;

        sector_state() {
            discard();
        }
        
        void discard(void) {
            locked = false;
            dirtied = false;
            delayed_clear = false;
            submitted = false;
            prev_dirty = NULL;
        }
        
        void clear(void) {
            dirtied = false;
            prev_dirty = NULL;
        }
        
        void write(void) {
            dirtied = false;
            delayed_clear = false;
            submitted = false;
            prev_dirty = NULL;
        }
    };

    void start(Log* l) {
        clear();
    }

    void finish(Log* l) {
        clear();
    }
    
    void clear(void) {
        states_t::iterator it;
        for_all_iter(it, _states)
            delete (*it).second;
        _states.clear();
    }

    sector_state* get_state(LinuxBlockRecPtr rec, int i) {
        sector_loc_t loc(rec->loc());
        loc.second += i;
        states_t::iterator it = _states.find(loc);

        if(it != _states.end())
            return (*it).second;

        sector_state *s;
        s = new sector_state;
        _states[loc] = s;
        return s;
    }

    void parse_lock(LinuxBlockRecPtr rec, Log* l) {
        int i;
        for(i=0; i<rec->nsector(); i++) {
            sector_state *s = get_state(rec, i);
            xi_assert(!s->locked, "sector %d is already locked!", 
                      rec->sector()+i);
            s->locked = true;
        }
    }
    void parse_unlock(LinuxBlockRecPtr rec, Log* l) {
        int i;
        for(i=0; i<rec->nsector(); i++) {
            sector_state *s = get_state(rec, i);
            if(!option_eq(fs, type, "xfs")) // why doesn't this work for xfs?
                xi_assert(s->locked, 
                          "unlocking sector %d that isn't locked!", 
                          rec->sector()+i);
            s->locked = false;
            if(s->delayed_clear) {
                s->clear();
                if(!s->submitted)
                    parse_remove_real(s->delayed_clear_rec, i, l);
                s->delayed_clear = false;
            }
        }
    }
    void parse_add(LinuxBlockRecPtr rec, Log* l) {
        int i;
        for(i=0; i<rec->nsector(); i++) {
            sector_state *s = get_state(rec, i);
            char* sector = rec->sector(i);
            signature_t sig = db.write(sector);

            if(rec->type() == EKM_BH_dirty)
                s->dirtied = true;
            else
                s->submitted = true;
            
            // skip redundant EKM_ADD_SECTOR
            if(s->prev_dirty && s->prev_dirty->sig() == sig)
                continue;
            
            l->push_back(new SectorRec(*rec, EKM_ADD_SECTOR, i, sig));
        }
    }
    void parse_remove(LinuxBlockRecPtr rec, Log* l) {
        int i;
        for(i=0; i<rec->nsector(); i++) {
            sector_state *s = get_state(rec, i);
#if 0
            // sometimes, they just call clear_buffer_dirty on a clean
            // buffer. e.g. unmap_underlying_metadata
            explode_assert(s->dirtied, "cleaning sector %d that isn't dirty!", 
                           rec->sector()+i);
#endif
            if(s->submitted) {
                s->clear();
                continue;
            }

            // if locked, delay add clear record until unlock because we
            // may submit it for write.  if that's the case, no need to
            // add the clear record
            if(s->locked) {
                s->delayed_clear = true;
                s->delayed_clear_rec = rec;
                continue;
            }

            s->clear();
            parse_remove_real(rec, i, l);
        }
    }

    void parse_remove_real(LinuxBlockRecPtr rec, int i, Log* l) {
        l->push_back(new SectorRec(*rec, EKM_REMOVE_SECTOR, i, 0));
    }

    void parse_write(LinuxBlockRecPtr rec, Log* l) {
        int i;
        for(i=0; i<rec->nsector(); i++) {
            sector_state *s = get_state(rec, i);
            char* sector = rec->sector(i);
            signature_t sig = db.write(sector);

            s->write();
            l->push_back(new SectorRec(*rec, EKM_WRITE_SECTOR, i, sig));
        }
    }

    void parse_read(LinuxBlockRecPtr rec, Log* l) {
        int i;
        for(i=0; i<rec->nsector(); i++) {
            sector_state *s = get_state(rec, i);
            char* sector = rec->sector(i);
            signature_t sig = db.write(sector);

            l->push_back(new SectorRec(*rec, EKM_READ_SECTOR, i, sig));
        }
    }

    states_t _states;

public:

    virtual void operator()(rec_type_t t, const char* &off, 
                            Log* l) {
        LinuxBlockRecPtr rec(new LinuxBlockRec(t, off));

        vn_out(4) << "parsing " << *rec << endl; 

        if(t == EKM_BH_dirty || t == EKM_BH_write
           || t == EKM_BH_write_sync || t == EKM_BIO_start_write)
            parse_add(rec, l);
        
        else if(t == EKM_BH_clean)
            parse_remove(rec, l);

        else if(t == EKM_BIO_write_done)
            parse_write(rec, l);

        else if(t == EKM_BH_lock)
            parse_lock(rec, l);

        else if(t == EKM_BH_unlock)
            parse_unlock(rec, l);

        else if(t == EKM_BIO_read)
            parse_read(rec, l);

        else if(t == EKM_BIO_write
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
                || t == EKM_BH_done
#endif
                )
            ; // just ignore
        
        else if(t == EKM_BH_wait || EKM_BIO_wait)
            ; // just ignore
    }

#define EKM(b, state) \
        LogMgr::the()->reg_existing_type(EKM_##b##_##state, #b"_"#state);

    static void reg(void) {
        LogMgr::the()->reg_existing_category(EKM_LINUX_CAT, "linux_buffer",
                                             new LinuxBlockRecParser);
        EKM(BH, dirty);
        EKM(BH, clean);
        EKM(BH, lock);
        EKM(BH, wait);
        EKM(BH, unlock);
        EKM(BH, write);
        EKM(BH, write_sync);


        EKM(BIO, start_write);
        EKM(BIO, wait);
        EKM(BIO, write);
        EKM(BIO, read);
        EKM(BIO, write_done);
    }
};

#endif //#ifndef __LINUX_LOG_H
