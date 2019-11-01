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
#include "hash_map_compat.h"

#include "crash.h"
#include "storage/Fs.h"
#include "storage/VMWare.h"
#include "driver/FsTestDriver.h"

using namespace std;
struct snapshot {
    string name;
    AbstFS fs;
};

enum snapshot_op { SNAP_CREATE, SNAP_DELETE };

struct hash_string {
    size_t operator()(const string& s) const {
        Sgi::hash<const char*> h;
        return h(s.c_str());
    }
};

struct eq_string {
    bool operator()(const string& s1, const string& s2) const {
        return strcmp(s1.c_str(), s2.c_str()) == 0;
    }
};
typedef Sgi::hash_map<string, struct snapshot*, 
                      hash_string, eq_string> snapshots_t;

class Test: public TestDriver {
private:
    HostLib *_ekm;
    NetLib *_remote_ekm;
    VMWare *_vm;
    Fs *_vm_fs;
    // FIXME: using string as key type introduces one allocation
    // and one deallocation whenever a char* is used as a key
    snapshots_t _snapshots;
    void create_snapshot(string &name);
    void add_snapshot(string &name);

public:
    Test(HostLib *ekm, NetLib *remote_ekm, VMWare *vm, Fs *vm_fs)
        :TestDriver(true), _ekm(ekm),
         _remote_ekm(remote_ekm), 
         _vm(vm), _vm_fs(vm_fs) {}
    void mutate(void);
    int check(enum commit_epoch_t, string, snapshot_op);
};
  
void Test::mutate(void)
{
    int it = 10;

    for(int i=0; i < 10; i++)
        _remote_ekm->systemf("touch test%02d", i);
    _remote_ekm->sync();

    for(int i = 0; i < it; i++) {
        string name; // we need to intialize this to be that number
        begin_commit(name, SNAP_CREATE);
        create_snapshot(name);
        end_commit(name, SNAP_CREATE);
        add_snapshot(name);
        // do more

    }

}

void Test::create_snapshot(string &name)
{
    _ekm->systemf("vmrun snapshot %s %s", _vm->path(), name.c_str());
}

void Test::add_snapshot(string &name)
{
    assert(_snapshots.find(name) == _snapshots.end());
    struct snapshot *snap = new struct snapshot;
    assert(snap);
    snap->name = name;
    // is the path right?
    _remote_ekm->abstract_path(_vm_fs->path(), snap->fs);
    _snapshots[name] = snap;
}

int Test::check(enum commit_epoch_t ct, string name, snapshot_op op)
{
    //assert(op == SNAP_CREATE);
    AbstFS fs;

    // for all snapshots that aren't != name, these should always match
    snapshots_t::iterator it;
    struct snapshot *snap;
    for(it = _snapshots.begin(); it != _snapshots.end(); it++) {
        snap = (*it).second;
        // restore snapshot = "vmrun revertToSnapshot <path> <name>"
        _ekm->systemf("vmrun revertToSnapshot %s %s",
                      _vm->path(), name.c_str());
        _remote_ekm->init();
        _remote_ekm->abstract_path(_vm_fs->path(), fs);
        if(snap->fs.compare(fs) != 0)
	    error("VMware Snapshot", "AbstractFS doesn't match");
    }
  
    if(ct == after_commit)
        ;

    /*switch(ct) {
      case begin_commit:
      break;

      case in_commit:
    

      case end_commit:
      // restores snapshot
      // get abstract fs
      // compare fs to _snapshot.fs*/
    return CHECK_SUCCEEDED;
}


void assemble(Component *&test, TestDriver *&driver)
{
    HostLib *ekm = sim()->new_ekm();
    NetLib *remote_ekm = sim()->new_net_ekm(get_option(ekm, net_server));

    set_option(rdd, sectors, 4096*64);
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

    driver = new Test(ekm, remote_ekm, vm, vm_ext3);
}
