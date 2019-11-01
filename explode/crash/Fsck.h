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


#ifndef __FSCK_H
#define __FSCK_H

#include <list>
#include <iostream>
#include "hash_map_compat.h"

#include "mcl_types.h"
#include "sector_loc.h"
#include "Component.h"
#include "BlockDev.h"

struct sector_record;
class Fsck {

    class node {
        friend class Fsck;
        
        bool leaf_p;
        sector_loc_t loc;
        wr_set_t wr_set;
        
        Sgi::hash_map<signature_t, node*> next;
        
        node() {leaf_p = false;}
    };
    
    node *root;
    
    void add_to_cache(std::list<sector_loc_t> rd_ord,
                      wr_set_t& rd_set,
                      wr_set_t& wr_set);
    std::ostream& print_cache(std::ostream& o, node* n, int level);
    
    void run_real(BlockDevDataVec& disks);
    int run_cached(BlockDevDataVec& disks);
    void clear(node *n);
    
    int num;
    
public:
    
    int hits, misses;
    
    Fsck(void) {root = NULL; hits = misses = 0;}
    ~Fsck(void);
    void run(BlockDevDataVec& disks);
    void test(void);
};

// only 1 copy of fsck_cache
extern Fsck fsck;
#endif
