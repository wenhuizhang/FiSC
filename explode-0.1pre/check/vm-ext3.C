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

#include "mcl.h"
#include "crash.h"
#include "driver/FsTestDriver.h"
#include "storage/VMWare.h"
#include "storage/storage-options.h"

void assemble(Component *&test, TestDriver *&driver)
{
    HostLib *ekm = sim()->new_ekm();
    NetLib *remote_ekm = sim()->new_net_ekm(get_option(ekm, net_server));

    set_option(rdd, sectors, 4096 * 64);
    // no VM rdds for now
    Rdd *d1 = sim()->new_rdd();
    Fs *fs = new_fs("/mnt/sbd0", ekm);
    VMWare *vm = new VMWare("/dev/sdb", ekm);
    Ext3 *vm_ext3 = new Ext3("/mnt/sbd0", remote_ekm);
    fs->plug_child(d1);
    vm->vmpath = "\"/home/junfeng/vmware/Virtual Machines/explode-new\"";
    vm->vmname = "explode-test.vmx";
    vm->vm_ekm = remote_ekm;
    vm->plug_child(fs);
    vm_ext3->plug_child(vm);
    vm_ext3->size(102*1024*1024);

    test = vm_ext3;

    driver = new_fs_driver(remote_ekm, vm_ext3);
    // NOTE: interface changed.  vm checking may not work. need to fix
}
