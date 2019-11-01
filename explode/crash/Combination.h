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


#ifndef __COMBINATION_H
#define __COMBINATION_H

#include <vector>

class Combination
{
public:
    typedef std::vector<int> set_t;

    Combination(int t, 
                int deter_thres, int rand_tries);
    Combination(set_t& ceilings, int t, 
                int deter_thres, int rand_tries);

    bool next(void);

    int iteration(void) const
    { return iter; }

    const set_t& operator*(void) const
    { return subset; }

    void ceilings(set_t& ceilings);

    void reset(void);

private:
    void rand_subset(set_t& subset);


    int type; // not used
    int deterministic_threshold;
    int random_tries;


    int total; // total number of subsets
    int iter; // number of subsets gone thru so far

    set_t ceils;
    set_t subset; 
};

std::ostream& operator<<(std::ostream&, const Combination::set_t subset);
#endif
