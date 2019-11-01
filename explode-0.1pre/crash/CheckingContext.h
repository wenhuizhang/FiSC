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


#ifndef __CHECKING_CONTEXT_H
#define __CHECKING_CONTEXT_H

#include "BlockDev.h"
#include "sector_loc.h"
#include "Log.h"

class CheckingContext {
public:
    CheckingContext(BlockDevDataVec& init_disks);
    ~CheckingContext(void);

    BlockDevDataVec& init_disks(void)
    { return _init_disks; }

    Log* log(void)
    { return _log; }

    // debugging support.  e.g. verify that *_p_init_disks + _log
    // = _final disks

    RawLogPtr _raw_log;
protected:
    Log* _log;
    BlockDevDataVec& _init_disks;
    BlockDevDataVec _final_disks; // for debugging
};

#endif
