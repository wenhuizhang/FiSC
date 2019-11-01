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


#include <sys/types.h>
#include <fstream>

#include "crash.h"
#include "CvsTestDriver.h"

using namespace std;

void CvsTestDriver::checkout(const string& dir, string& file) 
{
    // remove dangling locks
    _ekm->systemf("rm -rf %s/test-proj/#cvs.*", _cvs->path());
    // create working dir
    _ekm->systemf("rm  -rf %s", dir.c_str());
    _ekm->systemf("mkdir %s", dir.c_str());
    // check out
    _ekm->systemf("cd %s; cvs -d %s co test-proj", 
                  dir.c_str(), _cvs->path());
		
    file = dir + "/test-proj/test-file.txt";
}


void CvsTestDriver::mutate(void) {
    string dir = "/tmp/work", file;
    checkout(dir, file);
		
    // make sure everything is on disk before we check cvs commit
    sync();
		
    ofstream o(file.c_str());
    assert(o);
    char content[] = "test overwrite then commit";
    o << content;
    o.close();
		
    _ekm->systemf("cd %s; cvs commit -m \"test overwrite then commit\"", 
                  dir.c_str());
    // uncommenting this sync fixes the cvs error
    // sync();
    check_now_crashes(content, sizeof content - 1);
}

int CvsTestDriver::check(const char* content, int len) {
    string file = _cvs->path();
    file += "/test-file.txt";
		
    ifstream is(file.c_str());
    if(!is)
        error("cvscommit", "committed file gone");
		
    is.seekg (0, ios::end);
    int len2 = is.tellg();
		
    if(len != len2)
        error("cvscommit", "cvs commit file diff");
		
    char* buf = new char[len];
    assert(buf);
		
    is.seekg (0, ios::beg);
    is.read(buf, len);
    is.close();
		
    if(memcmp(buf, content, len))
        error("cvscommit", "cvs commit file diff");
		
    delete [] buf;
    return CHECK_SUCCEEDED;
}
