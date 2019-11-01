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


#ifndef __GENERIC_DEV_H
#define __GENERIC_DEV_H

#include "Component.h"

enum {
    GARBAGE_WORD = 0xabadface,
};

enum {
    NO_COPY = 0,
    COPY_DATA = 1,
};

// implements a generic disk-like device.  it could be RAID , RDD,
// or SMTD
class GenericDevData;
class GenericDev: public Component {

protected:
    // device ID.  uniquely identifies a block device, both in
    // user space and in kernel space.  The EKM log heavily uses
    // this ID.
    //
    // For Block Devices in Linux, this ID is the lower 32-bit of
    // the in-kernel ID dev_t.  The st.st_dev returned from stat()
    // uses a different encoding than the dev_t ID in Linux
    // Kernel, even the sizes of these two IDs are different.
    // However, the lower 32bits of st.st_dev and dev_t are
    // consistent.  So, we just use these 32 bits.  Check out
    // function BlockDev::BlockDev in BlockDev.C
    
    // For MTD, we use a fake major number
    int _id;

public:

    GenericDev(const char* path, HostLib* ekm):
        Component(path, ekm), _id(-1) {}

    int id(void) { return _id; }

    void start_trace(int want_read);
    void stop_trace(void);

    void kill(void); // mark this device as faulty
    void revive(void); // mart it as working

    virtual GenericDevData* create_data(int copy_data=NO_COPY);
    virtual void copy_from(GenericDevData *);
    virtual void copy_to(GenericDevData *);

    virtual void init(void);
};

class GenericDevData {
    friend class GenericDev;
protected:
    int _size; // size in bytes
    char *_data;

    GenericDev *_dev;

public:

    GenericDevData(GenericDev *dev, int copy_data);
    GenericDevData(GenericDevData& d);
    ~GenericDevData(void);

    int size(void) const {return _size;}
    char* data(void) {return _data;}
    GenericDev *dev(void) {return _dev;}

    // write to _data
    void write(long off, const char* d, int size);
    // read from _data
    void read(long off, char *d, int size);

    // set the contents of _dev 
    void set_dev_data(void)
    { _dev->copy_from(this); }
    // get the contents of _dev
    void get_dev_data(void)
    { _dev->copy_to(this); }

    // fill _data with magic value @magic
    void fill_with(unsigned magic);

    // dump _data to file
    void to_file(const char* file);
    // read _data from file
    void from_file(const char* file);

    virtual GenericDevData* clone(void)
    { return new GenericDevData(*this); }

    virtual GenericDevData& operator=(GenericDevData& s);
};

class GenericDevDataVec:public std::vector<GenericDevData*> {

protected:
    typedef std::vector<GenericDevData*> _Parent;
    GenericDevData* find_data(int dev);

public:

    ~GenericDevDataVec();
    void clear(void);
    
    void to_file(const char* dir, const char* prefix);
    void from_file(const char* dir, const char* prefix);

    GenericDevDataVec& operator=(const GenericDevDataVec& s);

    void set_dev_data(void);
    void get_dev_data(void);

    template<class Dev>
    void get_dev_data(std::vector<Dev*> disks) {
        int i;
        
        assert(size() == 0);
        resize(disks.size(), NULL);

        for_all(i, disks)
            (*this)[i] = disks[i]->create_data(COPY_DATA);
    }
};

#endif
