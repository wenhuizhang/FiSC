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


#include "mcl.h"
#include "CrashSim.h"
#include "Permuter.h"
#include "storage/Fs.h"
#include "driver/FsTestDriver.h"
#include "storage/Raid.h"
#include "ekm_lib.h"
#include <errno.h>

void assemble(Component *&test, TestDriver *&driver)
{
    HostLib *ekm = sim()->new_ekm();
    
    Rdd *d1 = sim()->new_rdd();
    Rdd *d2 = sim()->new_rdd();

    Raid *raid = new Raid("/dev/md0", ekm);
    raid->plug_child(d1);
    raid->plug_child(d2);

    Fs *fs = new_fs("/mnt/sbd0", ekm);
    fs->set_mnt_opts();
    fs->plug_child(raid);

    test = fs;

    driver = new_fs_driver(ekm, fs);
}
