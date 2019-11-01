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


#ifndef __REPLAY_MGR_H
#define __REPLAY_MGR_H

#include "hash_map_compat.h"

#include "Replayer.h"
#include "Log.h"

class ReplayMgr {
public:

    virtual ~ReplayMgr();
    void replay(Log*);

    void register_replayer(Replayer *r);
    void unregister_replayer(Replayer *r);

protected:

    typedef std::list<Replayer *> replayers_t;
    typedef Sgi::hash_map<rec_cat_t, replayers_t*>  replayer_registry_t;

    virtual void start_replay(void);
    virtual void finish_replay(void);

    replayers_t _replayers;
    replayer_registry_t _registry;
};

#endif
