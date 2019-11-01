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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <algorithm>
#include "Raid.h"
#include "ekm_lib.h"
#include "storage-options.h"

using namespace std;

void Raid::init(void)
{
	start_cmd = "sudo mdadm --assemble ";
	start_cmd += path();
	component_vec_t::size_type i;
	for(i = 0; i < children.size(); i ++) {
		start_cmd += " ";
		start_cmd += children[i]->path();
	}

	level = get_option(raid, level);
	set_default_size();

	char cmd[4096];
	sprintf(cmd, "sudo mdadm --create --verbose %s "\
			"--level=%s --raid-devices=%d",
			path(), level.c_str(), children.size());
	for(i = 0; i < children.size(); i ++) {
		strcat(cmd, " ");
		strcat(cmd, children[i]->path());
	}
	_ekm->systemf(cmd);
	umount();

	name = _path.substr(_path.rfind('/')+1);
}
		
void Raid::mount(void)
{
	_ekm->systemf(start_cmd.c_str());
	_ekm->systemf("sudo chown %s %s", getenv("USER"), path());
}

void Raid::umount(void)
{
	_ekm->systemf("sudo mdadm --stop %s", path());
}

void Raid::recover(void)
{
	//string cmd = "sudo mdadm --assemble --run --force --update=resync ";
	string cmd = "sudo mdadm --assemble --run --force ";
	cmd += path();
	component_vec_t::size_type i;
	for(i = 0; i < children.size(); i ++) {
		if(failed_children.find(i) != failed_children.end())
			continue;
		cmd += " ";
		cmd += children[i]->path();
	}
	_ekm->systemf(cmd.c_str());
	_ekm->systemf("sudo mdadm /dev/md0");

	for(i = 0; i < children.size(); i++) {
		if(failed_children.find(i) != failed_children.end()) {
			_ekm->systemf("sudo mdadm %s -a %s", path(), 
					 children[i]->path());
		}
	}
	failed_children.clear();
	_ekm->systemf("sudo mdadm --stop %s", path());
}

void Raid::threads(threads_t& ths)
{
	char th_name[MAX_PATH];
	sprintf(th_name, "%s_%s", name.c_str(), level.c_str());
	int pid = get_pid(th_name);
	if(pid > 0)
		ths.push_back(get_pid(th_name));	
}

void Raid::fail_child(int child_id)
{
	xi_assert(child_id >= 0 && child_id < (int)children.size(), 
				   "fail a child not in raid array %s", path());
	failed_children.insert(child_id);
}

void Raid::recover_child(int child_id)
{
	xi_assert(child_id >= 0 && child_id < (int)children.size(), 
				   "fail a child not in raid array %s", path());
	assert(failed_children.find(child_id) != failed_children.end());
	failed_children.erase(child_id);
}

void Raid::set_default_size(void)
{
	// TODO: compute raid size based on sizes of devices below
	int child_size;
	assert(children.size() > 0);
	child_size = children[0]->size();
	if(!size())
		_size = child_size - (128<<SECTOR_SIZE_BITS);
}

