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


#ifndef __CVS_H
#define __CVS_H

#include "Component.h"
#include <string>

class Cvs:public Component
{
private:
    std::string _name;
public:
    Cvs(const char* path, HostLib* ekm): 
        Component(path, ekm) {_in_kern = false;}

    const char* name() {return _name.c_str();}

    virtual void init(void) {
	int pos = _path.rfind('/');
	_name = _path.substr(pos+1);

        // check if cvs test repo exits
        struct stat st;
        std::string src = "test/";
        src += _name;
        if(stat(src.c_str(), &st) < 0)
            xi_panic("cvs test repo is not set up!");
        
	// copy test repository
	_ekm->systemf("cp -r test/%s %s", _name.c_str(), path());
    }
};
#endif
