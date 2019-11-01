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


#ifndef __Replayer_H
#define __Replayer_H

#include "ekm_log_types.h"
#include "Log.h"

// vector of categories
typedef std::vector<rec_cat_t> rec_cat_vec_t;

class Replayer {

public:
    virtual ~Replayer() {}

    virtual void start(void) {}
    virtual void finish(void) {}
    virtual void replay(Record *rec) {}

    const rec_cat_vec_t& interests(void) const 
    { return _interests; }

protected:
    rec_cat_vec_t _interests;
};

#endif
