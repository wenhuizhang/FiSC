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
#include "storage/Svn.h"

using namespace std;

class Test: public TestDriver
{
private:
    HostLib *_ekm;
    Svn *_svn;
    string dir;

    int sys(const char* cmd) {
        int nargs = 0;
        const char* p = cmd;
        while(p=strstr(p, "%s")) {
            nargs ++;
            p += 2;
        }
        switch(nargs) {
        case 1:
            return _ekm->systemf(cmd, dir.c_str());
        case 2:
            return _ekm->systemf(cmd, dir.c_str(), _svn->name());
        default:
            assert(0);
        }
    }

    void checkout(void) {
        sys("rm -rf %s");
        sys("mkdir -p %s");

        // checkout the project
        _ekm->systemf("cd %s && svn checkout file://%s", 
                      dir.c_str(), _svn->path());
    }

public:
    Test(HostLib *ekm, Svn *svn)
        :_ekm(ekm), TestDriver(true), _svn(svn), dir("/tmp/work") {}
    
    void mutate(void) {

        checkout();

        sys("touch %s/%s/trunk/file1");
        sys("cd %s/%s/trunk && svn add file1");
        sys("cd %s/%s/trunk && cat /etc/motd > file1");
        sync();

        // now commit the stuff
        sys("cd %s/%s && svn commit -m\"auto-change\"");

	//sync();
        check_now_crashes();
    }

    int check(void) {
        checkout();
        
                // diff the two files
        int err = sys("diff /etc/motd %s/%s/trunk/file1");
        if(0 != err)
            error("svn", "file1 differs");
        return CHECK_SUCCEEDED;
    }
};

void assemble(Component *&test, TestDriver *&driver)
{
    test = build_typical_fs_stack("/mnt/sbd0");

    Svn *svn = new Svn("/mnt/sbd0/svn-repo", test->ekm());
    assert(svn);
    svn->plug_child(test);

    test = svn;
	
    driver = new Test(test->ekm(), svn);
    assert(driver);
}
