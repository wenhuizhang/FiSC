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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fstream>

#include "crash.h"
#include "storage/Fs.h"

#define choose xe_random

using namespace std;

class Test: public TestDriver
{
public:
    void mutate(void);
    int check(void);
};

// *** begin here
const char *dir = "/mnt/sbd0/test-dir";
const char *file = "/mnt/sbd0/test-file";
// fsync the given file
static void do_fsync(const char *fn) {
    int fd = open(fn, O_RDONLY);
    fsync(fd);
    close(fd);
}
void Test::mutate(void) {
    switch(choose(4)) {
    case 0: systemf("mkdir %s%d", dir, choose(5)); break;
    case 1: systemf("rmdir %s%d", dir, choose(5)); break;
    case 2: systemf("echo \"test\" > %s", file);
        if(choose(2) == 0) 
            sync();
        else {
            do_fsync(file);
            do_fsync("/mnt/sbd0");
        }
	check_now_crashes(&Test::check);
        break;
    case 3: systemf("rm %s", file); break;
    }
}
int Test::check(void) {
    ifstream is(file);
    if(!is)
        error("fs", "file gone!");
    char buf[1024];
    is.read(buf, sizeof buf);
    is.close();
    if(strncmp(buf, "test", 4) != 0)
        error("fs", "file diff!");
    return 0;
}
void assemble(Component *&test, TestDriver *&driver)
{
    HostLib *ekm = sim()->new_ekm();
    Rdd *d1 = sim()->new_rdd();
    Fs *fs = new_fs("/mnt/sbd0", ekm);
    fs->plug_child(d1);
    driver = new Test;
    test = fs;
}
