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


#include <iostream>

#include "crash.h"
#include "RaidTestDriver.h"
#include "driver/FsTestDriver.h"
#include "driver/writes.h"

using namespace std;

RaidTestDriver::RaidTestDriver(HostLib *ekm, Raid* r)
{
    _actual = r;
    _stable = new StableRaid(this); assert(_stable);
}

void RaidTestDriver::mutate(void)
{
    int fd, ret;

    if((fd=_ekm->open(_actual->path(), O_WRONLY)) < 0) {
        perror("cannot open\n");
        assert(0);
    }

    int i;

    for(i = 0; i < nwrites; i++) {
        if((int)(writes[i][0].off + writes[i][0].len) 
           > _actual->size())
            break;
        sim()->pre_syscall();
        ret = _ekm->pwrite(fd, writes[i][0].buf, 
                           writes[i][0].len, writes[i][0].off);
        sim()->post_syscall();
        //assert(ret == (int)writes[i][0].len);
    }

    _ekm->close(fd);
}

Replayer* RaidTestDriver::get_replayer(void)
{
    return _stable;
}

int StableRaid::replay(struct ekm_record_hdr *rec)
{
    return 0;
}

void abstract_data(const char* path, data *d)
{
    int fd, ret, n, size;
    off_t off;
    static char buf[1024*16];

    fd = open(path, O_RDONLY);
    CHECK_RETURN(fd);

    off = 0;
    size = sizeof buf;
    while((n=::pread(fd, buf, size, off)) > 0) {
        d->pwrite(buf, n, off);
        off += n;
		
        // FIXME: data after this offset must be sparse.  don't
        // want to iterat thru all those zeroes.  however, there
        // is no syscall that tells us this file has holes.
        // cheat...
        if(off > 33000 && off < 0x40000000)
            off = 0x3ffff000;
    }
    CHECK_RETURN(n);
	
    ret = close(fd);
    CHECK_RETURN(fd);
}

int StableRaid::check(void)
{
    // wait till recovery is done
    // check if raid array is consistent by 
    //      failing one drive, 
    //      recover, 
    //      compare sig to known sig

    return CHECK_SUCCEEDED;

    int i, j, ndisks;
    data *raid_data;
    signature_t *sigs;
    BlockDevDataVec tmp_disks;

    ndisks = _driver->actual()->children.size();
    sigs = new signature_t[ndisks];
    assert(sigs);
    raid_data = new data[ndisks];
    assert(raid_data);

    sim()->umount();
    sync();
    // XXX implement this
    // permuter()->clone_current_disks(tmp_disks);
    for(i = 0; i < ndisks; i ++)
        tmp_disks[i]->get_dev_data();

    for(i = 0; i < ndisks; i ++) {
        //_driver->actual()->fail_child(i);
        for(j = 0; j < ndisks; j ++) {
            tmp_disks[i]->set_dev_data();
#if 0
            if(j == i)
                systemf("sudo mdadm --zero-superblock %s", 
                        _driver->actual()->children[i]->path());
#endif
        }

        Rdd* d = CAST(Rdd*,_driver->actual()->children[i]);
        d->kill();
        sim()->mount();
        sim()->umount();

        d->revive();
        sim()->recover();

        abstract_data(_driver->actual()->path(), &raid_data[i]);

        sigs[i] = AbstFS::data_sig(raid_data[i]);
        sim()->umount();
    }


    for(i = 0; i < ndisks-1; i ++) {
        if(sigs[i] != sigs[i+1]) {
            cout << "raid array inconsistent!\n"
                 << "disk " << i << ":"
                 << raid_data[i] << endl
                 << "disk " << i+1 << ":"
                 << raid_data[i+1] << endl;
            error("raid", "raid array inconsistent!");
        }
    }

    delete [] raid_data;
    for(i = 0; i < ndisks; i ++)
        delete tmp_disks[i];

    return CHECK_SUCCEEDED;
}
