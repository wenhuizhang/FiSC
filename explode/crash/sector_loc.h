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



#ifndef __SECTOR_LOC_H
#define __SECTOR_LOC_H

#include <utility>
#include <iostream>
#include "hash_map_compat.h"

#include "mcl.h"

// FIXME: don't use a pair.  just use a regular struct with
// meaningful field names
typedef std::pair<int, long> sector_loc_t;
static inline std::ostream& operator<<(std::ostream& o, const sector_loc_t& loc)
{
    return o << "dev=" << loc.first << "," 
             << "sector=" << loc.second;
}

// add writes, remove writes, lookup
struct hash_sector_loc {
    size_t operator()(const sector_loc_t& l) const {
        Sgi::hash<int> H1;
        Sgi::hash<unsigned long> H2;
        return H1(l.first) ^ H2(l.second);
    }
};

struct eq_sector_loc {
    bool 
    operator()(const sector_loc_t& l1, const sector_loc_t& l2) const {
        return l1.first == l2.first && l1.second == l2.second;
    }
};

struct lt_sector_loc {
    bool 
    operator()(const sector_loc_t& l1, const sector_loc_t& l2) const {
        return l1.first < l2.first 
            || (l1.first == l2.first && l1.second < l2.second);
    }
};

typedef Sgi::hash_map<sector_loc_t,
                      signature_t,
                      hash_sector_loc,
                      eq_sector_loc> wr_set_t;

static inline std::ostream& operator<<(std::ostream& o, const wr_set_t& wr_set)
{
    wr_set_t::const_iterator it;
    for_all_iter(it, wr_set) 
        o << "[" << (*it).first << ",sig=" << (*it).second << "] ";
    return o;
}

#endif
