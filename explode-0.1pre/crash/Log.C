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


#include "mcl.h"
#include "Log.h"
#include "CrashSim.h"
#include "ChunkDB.h"
#include "ekm_log_types.h"
#include "ekm/ekm.h"

using namespace std;

Record::Record(rec_type_t t, const char* &off): _type(t)
{
    _off = off;

    unmarshal(off, _lsn);
    unmarshal(off, _pid);
    unmarshal(off, _magic);

    if(_magic != EKM_LOG_MAGIC)
        xi_panic("log record magic number mismatch.  log corrupted");
}

Record::Record(rec_type_t t): _type(t)
{
    _lsn = 0;
    _pid = 0;
    _magic = EKM_LOG_MAGIC;
    _off = NULL;
}

ostream& Record::print(ostream& o) const
{
    return o << LogMgr::the()->name(_type) << ":"
             << "lsn=" << _lsn;
}

BlockRec::BlockRec(rec_type_t t, const char* &off, bool has_data)
    : Record(t, off)
{
    unmarshal(off, _loc.first);
    unmarshal(off, _flags);
    unmarshal(off, _loc.second);
    unmarshal(off, _size);
    if(has_data)
        unmarshal(off, _data, _size);
    else
        _data = NULL;
}

ostream& BlockRec::print(ostream& o) const
{
    return (Record::print(o))<<":" <<_loc<<":"<<_size;
}

BlockRec::~BlockRec(void)
{
    if(_data)
        delete[] _data;
}

SectorRec::SectorRec(const BlockRec& buf, int sector)
    : Record(*(static_cast<const Record*>(&buf)))
{
    // fix type
    switch(buf.type()) {
    case EKM_ADD_BLOCK:
        _type = EKM_ADD_SECTOR;
        break;
    case EKM_REMOVE_BLOCK:
        _type = EKM_REMOVE_SECTOR;
        break;
    case EKM_WRITE_BLOCK:
        _type = EKM_WRITE_SECTOR;
        break;
    }

    _loc.first = buf.loc().first;
    _loc.second = buf.loc().second + sector;

    _flags = buf.flags();

    _sig = db.write(buf.sector(sector));
}

SectorRec::SectorRec(const BlockRec& buf, rec_type_t type, 
                     int sector, const signature_t &sig)
    : Record(*(static_cast<const Record*>(&buf)))
{
    _type = type;
    _loc.first = buf.loc().first;
    _loc.second = buf.loc().second + sector;

    _flags = buf.flags();

    _sig = sig;
}

ostream& SectorRec::print(ostream& o) const
{
    return (Record::print(o))<<":"<<_loc<<":"<<_sig;
}

AppRec::AppRec(rec_type_t t, const char* &off): Record(t, off)
{
    unmarshal_array(off, _data, _size);
}

AppRec::AppRec(rec_type_t t): Record(t)
{
    _size = 0;
    _data = NULL;
}

AppRec::~AppRec(void)
{
    if(_data)
        delete[] _data;
}

void BlockRecParser::operator()(rec_type_t t, const char* &off, 
                                 Log* l)
{
    BlockRec *buffer = new BlockRec(t, off);
    int i;
    for(i=0; i<buffer->nsector(); i++)
        l->push_back(new SectorRec(*buffer, i));


    vn_out(4) << "parsing " << *buffer << endl; 

    delete buffer;
}

void BlockRecParser::reg(void)
{
    LogMgr::the()->reg_existing_category(EKM_BLOCK_CAT, "buffer",
                                         new BlockRecParser);
    LogMgr::the()->reg_existing_type(EKM_ADD_BLOCK, "add_buffer");
    LogMgr::the()->reg_existing_type(EKM_REMOVE_BLOCK, "remove_buffer");
    LogMgr::the()->reg_existing_type(EKM_WRITE_BLOCK, "write_buffer");
    LogMgr::the()->reg_existing_type(EKM_READ_BLOCK, "read_buffer");
}

void SectorRecParser::reg(void)
{
    LogMgr::the()->reg_existing_category(EKM_SECTOR_CAT, "sector",
                                         new SectorRecParser);
    LogMgr::the()->reg_existing_type(EKM_ADD_SECTOR, "add_sector");
    LogMgr::the()->reg_existing_type(EKM_REMOVE_SECTOR, "remove_sector");
    LogMgr::the()->reg_existing_type(EKM_WRITE_SECTOR, "write_sector");
    LogMgr::the()->reg_existing_type(EKM_READ_SECTOR, "read_sector");
}

void Log::clear(void)
{
    // delete log records
    iterator it;
    for_all_iter(it, *this)
        delete *it;

    _Parent::clear();
}
