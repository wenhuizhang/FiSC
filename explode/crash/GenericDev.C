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

#include "GenericDev.h"
#include "ChunkDB.h"
#include "Log.h"

using namespace std;

void GenericDev::init()
{
    struct stat st;
    if(stat(path(), &st) < 0)
        xi_panic("can't stat %s", path());
    assert((st.st_rdev & 0xffffffff00000000LL) == 0);
    _id = (int)st.st_rdev;
}

void GenericDev::start_trace(int want_read)
{
    int ret;
    assert(_ekm >= 0 && _id >= 0);
    if((ret=_ekm->ekm_start_trace(_id, want_read)) <  0)
        xi_panic("can't start trace %d", _id);
}

void GenericDev::stop_trace(void)
{
    assert(_ekm >= 0 && _id >= 0);
    if(_ekm->ekm_stop_trace(_id) <  0)
        xi_panic("can't stop trace %d", _id);
}

void GenericDev::kill(void)
{
    assert(_ekm >= 0 && _id >= 0);
    _ekm->ekm_kill_disk(_id);
}

void GenericDev::revive(void)
{
    assert(_ekm >= 0 && _id >= 0);
    _ekm->ekm_revive_disk(_id);
}

GenericDevData* GenericDev::create_data(int copy_data)
{
    return new GenericDevData(this, copy_data);
}

void GenericDev::copy_from(GenericDevData *d)
{
    xi_panic("not implemented yet!");
}

void GenericDev::copy_to(GenericDevData *d)
{
    xi_panic("not implemented yet!");
}

GenericDevData::GenericDevData(GenericDev *bd, int copy_data) 
{
    _dev = bd;
    _size = bd->size();
    _data = new char[_size];
    assert(_data);
    if(copy_data)
        bd->copy_to(this);
}

GenericDevData::GenericDevData(GenericDevData& d)
{
    _size = d.size();
    _dev = d.dev();
    if(d.data()) {
        _data = new char[_size];
        assert(_data);
        memcpy(_data, d.data(), _size);
    } else
        _data = NULL;
}

GenericDevData::~GenericDevData(void) 
{
    if(_data)
        delete [] _data;
}

void GenericDevData::fill_with(unsigned magic)
{
    assert(_size && _data);
    for(int i = 0; i < _size; i+= sizeof magic)
        if(_size - i < (int)sizeof magic)
            memcpy(_data+i, &magic, _size -i);
        else
            memcpy(_data+i, &magic, sizeof magic);
}

void GenericDevData::to_file(const char* file)
{
    int fd;
    if((fd=open(file, O_WRONLY|O_CREAT, 0777)) < 0) {
        perror("open");
        assert(0);
    }
    int count = 0, ret;
    while(count < size()) {
        if((ret=::write(fd, data()+count, size()-count)) < 0) {
            perror("write");
            assert(0);
        }
        count += ret;
    }
    close(fd);
}

void GenericDevData::from_file(const char* file)
{
    int fd;
    if((fd=open(file, O_RDONLY, 0777)) < 0) {
        perror("open");
        assert(0);
    }
    int count = 0, ret;
    while(count < size()) {
        if((ret=::read(fd, data()+count, size()-count)) < 0) {
            perror("read");
            assert(0);
        }
        count += ret;
    }
    close(fd);
}

void GenericDevData::write(long off, const char* d, int s)
{
    assert(off >= 0 && off < size());
    memcpy(data() + off, d, s);
}

void GenericDevData::read(long off, char* d, int s)
{
    assert(off >= 0 && off < size());
    memcpy(d, data() + off, s);
}

// copy disk contents of @d
GenericDevData& GenericDevData::operator=(GenericDevData& s)
{
    assert(_size == s.size()
           && _data && s.data()
           && _dev == s.dev());
    memcpy(_data, s.data(), _size);
    return *this;
}

GenericDevDataVec::~GenericDevDataVec()
{
    clear();
}

GenericDevData* GenericDevDataVec::find_data(int dev_id)
{
    iterator it;

    for_all_iter(it, *this) {
        if((*it)->dev()->id() == dev_id)
            return *it;
    }

    xi_warn("can't find GenericDevData object for dev %d", dev_id);
    return NULL;
}

void GenericDevDataVec::clear(void)
{
    iterator it;
    for_all_iter(it, *this)
        if(*it) delete *it;
    _Parent::clear();
}

void GenericDevDataVec::to_file(const char* dir, const char* prefix)
{
    int i;
    static char name[4096];
    for_all(i, *this) {
        sprintf(name, "%s/%s-rdd%d", dir, prefix, i);
        (*this)[i]->to_file(name);
    }
}

void GenericDevDataVec::from_file(const char* dir, const char* prefix)
{
    int i;
    static char name[4096];
    for_all(i, *this) {
        sprintf(name, "%s/%s-rdd%d", dir, prefix, i);
        (*this)[i]->from_file(name);
    }
}

// copy our array of disk contents to @dst
GenericDevDataVec& GenericDevDataVec::operator=(const GenericDevDataVec& s)
{
    int i;

    if(size() == 0) {
        // first assignment, allocate disk data
        resize(s.size(), NULL);
        for_all(i, *this)
            (*this)[i] = s[i]->clone();

        return *this;
    }

    xi_assert(size() == s.size(),
              "source and destination have different sizes!");

    for_all(i, *this)
        // disk data already allocated, simply copy
        *(*this)[i] = *s[i];
    return (*this);
}

void GenericDevDataVec::set_dev_data(void)
{
    iterator it;
    for_all_iter(it, *this)
        (*it)->set_dev_data();
}

void GenericDevDataVec::get_dev_data(void)
{
    iterator it;
    for_all_iter(it, *this)
        (*it)->get_dev_data();
}
