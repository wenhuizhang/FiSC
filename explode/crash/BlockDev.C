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


#include "BlockDev.h"
#include "ChunkDB.h"
#include "Log.h"

GenericDevData* BlockDev::create_data(int copy_data)
{
    return new BlockDevData(this, copy_data);
}

BlockDevData::BlockDevData(BlockDev *bd, int copy_data) 
    : GenericDevData(bd, copy_data)
{
    _sectors = _size>>SECTOR_SIZE_BITS;
}

BlockDevData::BlockDevData(BlockDevData& d)
    : GenericDevData(d)
{
    assert((int)(d.sectors()*SECTOR_SIZE) == d.size());
    _sectors = d.sectors();
}

void BlockDevData::write_sector(long sector, const char* d)
{
    assert(sector >= 0 && sector < (long)sectors());
    memcpy(data() + (sector*SECTOR_SIZE), d, SECTOR_SIZE);
}

void BlockDevData::read_sector(long sector, char* d)
{
    assert(sector >= 0 && sector < (long)sectors());
    memcpy(d, data() + (sector*SECTOR_SIZE), SECTOR_SIZE);
}

bool BlockDevDataVec::have_sector(const SectorRec *s)
{
    signature_t sig;
    read(s->loc(), sig);
    return sig == s->sig();
}

void BlockDevDataVec::write(const SectorRec *s)
{
    BlockDevData *disk = static_cast<BlockDevData*>(find_data(s->device()));

    vv_out<< "write dev=" << s->device()
          << ", sector=" << s->sector()
          << ", hash=" << s->sig()
          << "\n";

    disk->write_sector(s->sector(), db.read(s->sig()));
}

void BlockDevDataVec::write(const sector_loc_t& loc, const signature_t& sig)
{
    BlockDevData *disk = static_cast<BlockDevData*>(find_data(loc.first));

    vv_out<< "write dev=" << loc.first
          << ", sector=" << loc.second
          << ", hash=" << sig
          << "\n";

    disk->write_sector(loc.second, db.read(sig));
}

void BlockDevDataVec::read(const sector_loc_t& loc, signature_t& sig)
{
    static char buf[SECTOR_SIZE];
    BlockDevData *disk = static_cast<BlockDevData*>(find_data(loc.first));

    disk->read_sector(loc.second, buf);
    sig = db.write(buf);

    vv_out<< "read dev=" << loc.first
          << ", sector=" << loc.second
          << ", hash=" << sig//hash3((ub1*)w->s_data, SECTOR_SIZE, 0)
          << "\n";
}

void BlockDevDataVec::write_set(const wr_set_t& wset)
{
    wr_set_t::const_iterator i;
    for_all_iter(i, wset)
        write((*i).first, (*i).second);
}

void Rdd::init(void)
{
    BlockDev::init();

    // create a handle to this RDD
    _handle = _ekm->open_rdd(path());
    assert(_handle >= 0);
 
    // cannonicalize rdd data to contain some fixed magic values
    BlockDevData *data = static_cast<BlockDevData*>(create_data(NO_COPY));
    data->fill_with(GARBAGE_WORD);
    copy_from(data);
    delete data;
}

Rdd::~Rdd(void)
{
    if(_handle >= 0)
        _ekm->close_rdd(_handle);
}

void Rdd::copy_from(GenericDevData *d)
{
    assert(d->size() && d->data() && _handle >= 0);
    /*std::cout << "rdd hash=" 
              << explode_hash((ub1*)d->data(), d->size()) 
              << "\tdata=" << (void*)d->data() 
              << "\tsize=" << d->size()
              << "\thandle="<<_handle << std::endl; */
    if(_ekm->rdd_set_disk(_handle, d->data(), d->size()) < 0)
        xi_panic("can't set disk to %d", _handle);
}

void Rdd::copy_to(GenericDevData *d)
{
    assert(d->size() && d->data() && _handle >= 0);
    /*std::cout << "rdd hash=" 
              << explode_hash((ub1*)d->data(), d->size()) 
              << "\tdata=" << (void*)d->data()
              << "\tsize=" << d->size()
              << "\thandle="<<_handle << std::endl;*/
    if(_ekm->rdd_get_disk(_handle, d->data(), d->size()) < 0)
        xi_panic("can't get disk from %d", _handle);
}
