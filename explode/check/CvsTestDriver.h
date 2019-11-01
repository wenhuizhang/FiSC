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


#ifndef __CVSTESTDRIVER_H
#define __CVSTESTDRIVER_H

#include "driver/TestDriver.h"
#include "storage/Cvs.h"

class CvsTestDriver: public TestDriver {
private:
    HostLib *_ekm;
    Cvs *_cvs;
    void checkout(const std::string& dir, std::string& file);
	
public:
    CvsTestDriver(HostLib *ekm, Cvs *cvs)
        :_ekm(ekm), TestDriver(true), _cvs(cvs) {}

    void mutate(void);
    int check(const char* content, int len);
};

#endif
