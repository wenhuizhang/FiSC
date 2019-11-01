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


#ifndef __RAID_H
#define __RAID_H

#include "BlockDev.h"

class Raid: public BlockDev {
 protected:
	int min_size;
	std::string start_cmd;
	std::set<int> failed_children;
	std::string level;
	std::string name;

 public:
	Raid(const char* path, HostLib* ekm): 
		BlockDev(path, ekm) {}
	// given size for the raid device, compute the size of the
	// underlying disks
	int disk_size(int raid_size);

	virtual void set_default_size(void);

	virtual void init(void);
	virtual void mount(void);
	virtual void umount(void);
	virtual void recover(void);
	virtual void threads(threads_t&);

	void fail_child(int child_id);
	void recover_child(int child_id);
};

#endif
