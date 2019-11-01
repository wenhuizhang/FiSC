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


#include <errno.h>
#include <fstream>
#include "crash.h"
#include "storage/Fs.h"
#include "driver/TestDriver.h"
using namespace std;
class Cvs:public Component {// Class %%\v{Cvs}%% inherits %%\v{Component}%%
public:
    Cvs(const char* path, HostLib* ekm) // 
        : Component(path, ekm) {_in_kern = false;}
    virtual void init(void) {
	// copy a CVS repository (%%\v{cvs-test-repo}%%) from the current
	// directory to %%\v{path()}%%, a directory on a %%\xxx%'s RAM disk.
        // %%\v{cvs-test-repo}%% contains one project %%\v{test-proj} with one empty file %%\v{test-file.txt}%%
	_ekm->systemf("cp -r ./cvs-test-repo %s", path());
    }
    // 
};
class Test: public TestDriver {
public:
    Test(HostLib *ekm, Cvs *cvs)
        :_ekm(ekm), TestDriver(true), _cvs(cvs) {}
    void mutate(void) {
        string dir = "/tmp/work", file; // "/tmp/work" is our working directory
        checkout(dir, file); // check out %%\v{test-proj}%% to "/tmp/work"
        sync(); // make sure everything is sync'ed to disk before we do anything

        // Change the contents of the test file
        ofstream o(file.c_str()); assert(o);
        char content[] = "test overwrite then commit";
        o << content;
        o.close();

        // Commit the change
        _ekm->systemf("cd %s; cvs commit -m \"test overwrite then commit\"", dir.c_str());
        // sync(); //Uncommenting this sync fixes the cvs error
        // Invoke %%\xxx%% to check for crashes
        check_now_crashes(content, sizeof content - 1);
    }
    int check(const char* content, int len) {
        string file = _cvs->path();
        file += "/test-file.txt";
	// Check test-file.txt exists
        ifstream is(file.c_str());
        if(!is) error("cvscommit", "committed file gone");
	// Check the length is correct
        is.seekg (0, ios::end);
        int len2 = is.tellg();
        if(len != len2) error("cvscommit", "committed file size wrong");
        // Check the contents is correct
        char* buf = new char[len]; assert(buf);
        is.seekg (0, ios::beg);
        is.read(buf, len);
        is.close();
        if(memcmp(buf, content, len))
            error("cvscommit", "committed file contents wrong");
        delete [] buf;
        return CHECK_SUCCEEDED;
    }
private:
    HostLib *_ekm;
    Cvs *_cvs; // Pointer to a Cvs storage component
    // Check out %%\v{test-proj}%% into %%\v{dir}%%
    void checkout(const std::string& dir, std::string& file) {
        // remove dangling locks
        _ekm->systemf("rm -rf %s/test-proj/#cvs.*", _cvs->path());
        // create the working dir
        _ekm->systemf("rm  -rf %s", dir.c_str());
        _ekm->systemf("mkdir %s", dir.c_str());
        // check out
        _ekm->systemf("cd %s; cvs -d %s co test-proj", 
                      dir.c_str(), _cvs->path());
        // set file name to later reference
        file = dir + "/test-proj/test-file.txt";
    }
};
// Build a stack with CVS on top of ext3 on top of a %%\xxx%% RAM disk
void assemble(Component *&test, TestDriver *&driver) {
    Fs *fs = (Fs*)build_typical_fs_stack("/mnt/sbd0");
    Cvs *cvs = new Cvs("/mnt/sbd0/cvs-test-repo", fs->ekm()); assert(cvs);
    cvs->plug_child(fs);
    test = cvs;
    driver = new Test(test->ekm(), cvs); assert(driver);
}
