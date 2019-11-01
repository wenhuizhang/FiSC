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


#ifndef __POWERSET_H
#define __POWERSET_H

#include<set>
#include<bitset>
enum {atomic = 1, left_to_right = 2, unordered = 3, interleaved = 4};
#define MAX_SET (10)

class Powerset
{
public:
    typedef std::set<int> set_t;
	
    Powerset(int t, 
             int deter_thres, int rand_tries)
    { init(0, t, deter_thres, rand_tries); }

    void init(int n, int t, int deter_thresh, int rand_tries) {
        iter = 0;
        size = n;
        type = t;
        deterministic_threshold = deter_thresh;
        random_tries = rand_tries;
        skip_table.reset();
    }

    void init(int n) {
        size = n;
        skip_table.reset();
    }

    bool next(void);

    int iteration(void) const
    { return iter; }

    const set_t& operator*(void) const
    { return subset; }

    void skip_prefix(void);

private:

    static void gen_powerset(void);
    static void gen_powerset_(unsigned n, unsigned& s, 
                              std::vector<set_t>& powerset);
    static bool generated;
    static std::vector<set_t> powerset;

    void rand_subset(set_t& subset);

    int deterministic_threshold;
    int random_tries;
	
    int size;
    int iter;
    int type;

    set_t subset; 

    std::bitset<1<<MAX_SET> skip_table;

};

std::ostream& operator<<(std::ostream& o, const Powerset::set_t subset);

#endif
