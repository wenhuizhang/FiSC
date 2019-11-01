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


#ifndef __TESTDRIVER_H
#define __TESTDRIVER_H

#include <string>

#include "mcl_types.h"
#include "Replayer.h"

enum {
    CHECK_ABORT=-1, // didn't finish checking.
    CHECK_FAILED=0,  // found an error!
    CHECK_SUCCEEDED=1,
};

class TestDriver
{
protected:
    bool _new_interface_p;

public:
    TestDriver(bool b): _new_interface_p(b) {}
    TestDriver(): _new_interface_p(false) {}
    virtual ~TestDriver() {}

    bool new_interface_p(void) const {
        return _new_interface_p;
    }

    virtual void mount(void) {}
    virtual void umount(void) {}
    virtual void mutate(void) {}
    virtual signature_t get_sig(void)
    { return 0; }

    // new interface
    virtual int check(void) 
    { return CHECK_SUCCEEDED; }

    // old interface
    virtual Replayer* get_replayer(void)
    { return NULL; }
};



#endif
