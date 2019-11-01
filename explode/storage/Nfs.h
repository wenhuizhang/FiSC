/* -*- Mode: C++ -*- */

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


#ifndef __NFS_H
#define __NFS_H

#include "Fs.h"

class Nfs: public Fs {
public:
    Nfs(const char* path, bool testp, HostLib* ekm): 
        Fs(path, ekm) {
        local_p = false;
    }

    virtual void mount(void) {
	_ekm->systemf("sudo /etc/init.d/nfs-kernel-server start");
	_ekm->systemf("sudo mount -t nfs %s, %s %s", 
                      mnt_opt.c_str(), children[0]->path(), path());
	_ekm->systemf("sudo chown -R %s %s", getenv("USER"), path());
    }

    virtual void umount(void) {
	_ekm->systemf("sudo umount %s", path());
    }

    virtual bool ignore(const char* path) {
	assert(children.size() == 1);
	Fs *child_fs = dynamic_cast<Fs*>(children[0]);
	explode_assert(child_fs, "NFS must run on top of a fs");
	return child_fs->ignore(path);
    }
};

#endif
