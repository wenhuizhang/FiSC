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


#ifndef __VMWARE_H
#define __VMWARE_H

#include "Component.h"
#include "NetLib.h"
#include "storage-options.h"

class VMWare: public Component {
public:
    std::string vmpath;
    std::string vmname;
    NetLib *vm_ekm;

    VMWare(const char* path, HostLib* ekm): 
        Component(path, ekm) {}

    virtual void init(void) {
	if(get_option(vmware, compact)) {
            // compact disk
            _ekm->systemf("cp %s/explode-2.vmdk %s/explode-0.vmdk",
                          vmpath.c_str(), children[0]->path());
            _ekm->systemf("cp %s/explode-2-*.vmdk %s",
                          vmpath.c_str(), children[0]->path());
	}
	else // pre-allocated disk
            _ekm->systemf("cp %s/explode-0*.vmdk %s",
                          vmpath.c_str(), children[0]->path());
    }

    virtual void mount(void) {
	// start the vm
	_ekm->systemf("cp %s/%s %s/explode.vmx", 
                      vmpath.c_str(), vmname.c_str(), vmpath.c_str());
	_ekm->systemf("vmware-cmd %s/explode.vmx start", 
                      vmpath.c_str(), vmname.c_str());
	vm_ekm->init();
    }

    virtual void umount(void) {
	_ekm->systemf("vmware-cmd %s/explode.vmx stop hard", vmpath.c_str());
    }

    // when vmware starts, it'll automatically recover
    //virtual void recover(void) {}
};

#endif
