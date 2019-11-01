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
#include "storage/Fs.h"
#include "storage/VMWare.h"

#include "driver/FsTestDriver.h"

int main(void)
{
    systemf("sudo ./unload");

    load_options("explode.options");

    HostLib *ekm = new HostLib;
    ekm->load_ekm();

    NetLib *remote_ekm = new NetLib(get_option(ekm, net_server));

    // load rdd, for now, no VM rdds
    int nsectors = 4096 * 64;
    int size_in_bytes = nsectors * 512;
    ekm->load_rdd(1, nsectors);

    Rdd *d1 = new Rdd("/dev/rdd0", ekm);
    Ext3 *ext3 = new Ext3("/mnt/sbd0", ekm);
    VMWare *vm = new VMWare("/dev/sdb", ekm);
    Ext3 *vm_ext3 = new Ext3("/mnt/sbd0", remote_ekm);
    ext3->plug_child(d1);
    vm->vmpath = "\"/home/junfeng/vmware/Virtual Machines/explode-new\"";
    vm->vmname = "explode-test.vmx";
    vm->vm_ekm = remote_ekm;
    vm->plug_child(ext3);
    vm_ext3->plug_child(vm);
    vm_ext3->size(102*1024*1024);

    // set disk size in sectors
    d1->sectors(nsectors);

    vm_ext3->init_all();

    remote_ekm->sync();
    remote_ekm->systemf("mkdir /mnt/sbd0/test");
    for(int i=0; i < 10000; i ++)
        remote_ekm->systemf("touch /mnt/sbd0/test_file%02d", i);
    remote_ekm->sync();
	
    BlockDevData *data = new BlockDevData(d1, COPY_DATA);

    vm_ext3->umount_all();
	
    data->set_dev_data();

    vm_ext3->recover_all();
    return 0;
}
