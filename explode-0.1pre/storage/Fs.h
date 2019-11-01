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


#ifndef __FS_H
#define __FS_H

#include "Component.h"

Component *build_typical_fs_stack(const char* mnt);

class Fs: public Component {
 public:
	// vars for this file system type.  (should be static
	// members because all instanaces of one fs type share the
	// same properties, but can't do this because all child
	// classes share the same static member with parent
	// class.)
	int min_num_blks;
	int num_blks; // current number of blocks
	int blk_size; // current block size
	std::vector<int> supported_blk_sizes; // supported block sizes
	std::string type; // file system type
	bool fat_p; // msdos/vfat is different.  can only be
		      // accessed as root (e.g. msdos, vfat).
		      // file permision is fixed (0744)
	bool root_p; // must be root to mount this fs.  applies to
		     // ms fs and hfs
	bool hole_p; // does it support hole in file?
	bool local_p; // local fs or network fs

	// vars for file system instances
	std::string mnt_opt; // mount options
	bool mount_sync_p;
	enum fsck_mode fsck_mode; // current fsck mode
	std::vector<enum fsck_mode> supported_fsck_modes;

	void set_default_blk_size(void);
	void set_default_fsck_mode(void);
	void check_config(void);


	Fs(const char* path, HostLib* ekm): 
		Component(path, ekm) {
		min_num_blks = num_blks = blk_size = 0;
		fat_p = false;
		hole_p = true;
		local_p = true;
	};

	virtual void init(void);
	virtual void mount(void);
	virtual void umount(void);

	// paths to ignore when comparing two file systems, or
	// choosing a directory to create new files/dirs, or
	// abstracting. This function should be used by AbstFS
	virtual bool ignore(const char* path) {
		if(strstr(path, "lost+found"))
			return true;
		return false;
	}

	void append_mnt_opt(std::string opt);
	int get_blk_size(void) {return blk_size;}

	// set mount options common to all file systems based on
	// explode.options.  don't call this function if you don't
	// want these options.(e.g. when checking CVS)
	virtual void set_mnt_opts(void);
	virtual mode_t get_mode(void) { return 0777;}
};

Fs *new_fs(const char* mnt, HostLib* ekm_handle);

class Ext2: public Fs {
 public:
	Ext2(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	// no thread associated with ext2 except the system-wide
	// pdflush daemon, and we disabled pdflush
	// virtual void threads(thread_t&);
};

#define EEEE
#ifdef EEEE
class Ext3: public Fs {
 public:
	Ext3(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	virtual void threads(threads_t&);

	virtual void set_mnt_opts(void);
};
#else
class Ext3: public Fs {
 public:
	Ext3(const char* path);
	Ext3(const char* path, HostLib* ekm);
	virtual void init(void);
	virtual void recover(void);
	virtual void threads(threads_t&);

	virtual void mount(void);
	virtual void umount(void);
};
#endif

class Jfs: public Fs {
 public:
	Jfs(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	virtual void threads(threads_t&);
};

class ReiserFS: public Fs {
 public:
	ReiserFS(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	virtual void threads(threads_t&);

	virtual void set_mnt_opts(void);
};

class Reiser4: public Fs {
 public:
	Reiser4(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	virtual void threads(threads_t&);
};

class Xfs: public Fs {
 public:
	Xfs(const char* path, HostLib* ekm);
	
	virtual void init(void);
	virtual void recover(void);
	virtual void threads(threads_t&);

	virtual void set_mnt_opts(void);
};

class msdos: public Fs {
 public:
	msdos(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	// no threads
	virtual mode_t get_mode(void) { return 0755;}
};

class vfat: public Fs {
 public:
	vfat(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	// no threads
	virtual mode_t get_mode(void) { return 0755;}
};

class ntfs: public Fs {
 public:
	ntfs(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	// no threads
	virtual mode_t get_mode(void) { return 0755;}
};

class hfs: public Fs {
 public:
	hfs(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	// no threads
	virtual mode_t get_mode(void) { return 0700;}
};

class hfsplus: public Fs {
 public:
	hfsplus(const char* path, HostLib* ekm);

	virtual void init(void);
	// no threads
	virtual void recover(void);
};

class minix: public Fs {
 public:
	minix(const char* path, HostLib* ekm);

	virtual void init(void);
	virtual void recover(void);
	// no threads

	virtual void set_mnt_opts(void);
};

//freebsd file systems

class ufs: public Fs {
 public:
	ufs(const char* path, HostLib* ekm);
	
	virtual void init(void);
	virtual void recover(void);
	// no threads
};

#endif
