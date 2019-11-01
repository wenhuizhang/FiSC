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
#include "CvsTestDriver.h"

using namespace std;

void assemble(Component *&test, TestDriver *&driver)
{
    test = build_typical_fs_stack("/mnt/sbd0");

    Cvs *cvs = new Cvs("/mnt/sbd0/cvs-test-repo", test->ekm());
    assert(cvs);
    cvs->plug_child(test);

    test = cvs;
	
    driver = new CvsTestDriver(test->ekm(), cvs);
    assert(driver);
}
