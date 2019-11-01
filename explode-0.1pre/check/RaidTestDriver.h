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


#ifndef __BDEVTESTDRIVER_H
#define __BDEVTESTDRIVER_H

#include "driver/TestDriver.h"
#include "storage/Raid.h"

class StableRaid;
class RaidTestDriver: public TestDriver
{ 
private:
    HostLib *_ekm;
    Raid *_actual;
    StableRaid *_stable;

public:
    RaidTestDriver(HostLib *ekm, Raid* r);
    virtual void mutate(void);
    virtual Replayer *get_replayer(void);
	
    Raid *actual() {return _actual;}
    StableRaid *stable(){return _stable;}

};

class StableRaid: public Replayer
{
private:
    RaidTestDriver *_driver;
public:
    StableRaid(RaidTestDriver *d) {_driver = d;}
    // returns true to run permutation check
    virtual int replay(struct ekm_record_hdr *rec);
    virtual int check(void);
};

#endif
