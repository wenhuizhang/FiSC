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


#ifndef __SVN_H
#define __SVN_H

#include <string>
#include "Component.h"
#include "storage-options.h"

using namespace std;

class Svn :public Component {
private:
    std::string _name;
public:
    Svn(const char* path, HostLib* ekm): 
        Component(path, ekm) {_in_kern = false;}

    const char* name() {return _name.c_str();}

    virtual void init(void) {
        const char *workdir = "/tmp/workdir";
        const char *projdir = "proj";
    
        int pos = _path.rfind('/');
        _name = _path.substr(pos+1);

        // create repository
        _ekm->systemf("mkdir -p %s", path());
        _ekm->systemf("svnadmin create --fs-type %s %s", 
                      get_option(svn, type), path());

        // import project
        _ekm->systemf("rm -rf %s", workdir);
        _ekm->systemf("mkdir -p %s", workdir);
        _ekm->systemf("mkdir -p %s/%s/trunk", workdir, projdir);
        _ekm->systemf("mkdir %s/%s/branches", workdir, projdir);
        _ekm->systemf("mkdir %s/%s/tags", workdir, projdir);
    
        _ekm->systemf("cd %s && svn import -m\"initial-import\" %s file://%s",
                      workdir, projdir, path());
    }

    virtual void recover(void) {
        _ekm->systemf("svnadmin recover %s", path());
    }
};
#endif
