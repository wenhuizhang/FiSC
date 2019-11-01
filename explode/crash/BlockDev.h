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


#ifndef __BLOCK_DEV_NEW_H
#define __BLOCK_DEV_NEW_H

#include "sector_loc.h"
#include "GenericDev.h"

class BlockDev: public GenericDev
{
public:
    BlockDev(const char* path, HostLib* ekm):
        GenericDev(path, ekm) {}

    void sectors(int nsec) {
        _size = (nsec*SECTOR_SIZE);
    }
    int sectors(void) {
        return _size/SECTOR_SIZE;
    }
    
    virtual GenericDevData *create_data(int copy_data=NO_COPY);
};

class BlockDevData: public GenericDevData {

protected:
    int _sectors;  // size in sectors

public:
    BlockDevData(BlockDev *bd, int copy_data);
    BlockDevData(BlockDevData& d);

    void write_sector(long sector, const char* d);
    void read_sector(long sector, char* d);
    
    int sectors(void) const { return _sectors;}

    virtual GenericDevData* clone(void)
    { return new BlockDevData(*this); }

};

class SectorRec;
class BlockDevDataVec: public GenericDevDataVec {
public:
    bool have_sector(const SectorRec *wr);
    void write(const SectorRec *wr);
    void write(const sector_loc_t& loc, 
               const signature_t& sig);
    void read(const sector_loc_t& loc, 
              /* out */ signature_t& sig);
    void write_set(const wr_set_t& wset);
};

// wrapper for RDD
class Rdd: public BlockDev {

protected:
    int _handle; // openend fd for this RDD.  used to do ioctls

public:
    Rdd(const char* path, HostLib* ekm): BlockDev(path, ekm), _handle(-1) {}
    Rdd();
    virtual ~Rdd(void);

    virtual void copy_to(GenericDevData *data);
    virtual void copy_from(GenericDevData *data);

    virtual void init(void);
};

#endif
