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


#include <assert.h>

#include "util.h"
#include "ReplayMgr.h"
#include "Log.h"

using namespace std;

void ReplayMgr::start_replay(void)
{
    replayers_t::iterator it;
    for_all_iter(it, _replayers)
        (*it)->start();
}

void ReplayMgr::finish_replay(void)
{
    replayers_t::iterator it;
    for_all_iter(it, _replayers)
        (*it)->finish();
}

void ReplayMgr::replay(Log* log)
{
    Log::iterator it;

    start_replay();
    for_all_iter(it, *log) {
        Record *rec = *it;
        if(exists(rec->category(), _registry)) {
            replayers_t::iterator it;
            for_all_iter(it, *_registry[rec->category()])
                (*it)->replay(rec);
        } else {
            vvv_out << "no replayer for record " 
                    << LogMgr::the()->name(rec) << endl;
        }
    }
    finish_replay();
}

void ReplayMgr::register_replayer(Replayer *r)
{
    int i;
    replayers_t *v;
    const rec_cat_vec_t& cat = r->interests();

    xi_assert(cat.size(), "replayer has no interests!");

    for_all(i, cat) {
        if(!exists(cat[i], _registry)) {
            v = new replayers_t;
            _registry[cat[i]] = v;
        }
        else
            v = _registry[cat[i]];
        assert(v);
        v->push_back(r);
    }

    _replayers.push_back(r);
}

void ReplayMgr::unregister_replayer(Replayer *r)
{
    // TODO
}

ReplayMgr::~ReplayMgr()
{
    replayer_registry_t::iterator it;
    for_all_iter(it, _registry)
        delete (*it).second;

    // we don't own the replayers, so it's not our responsibility
    // to delete them
}
