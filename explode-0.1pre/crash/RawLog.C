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


#include "ekm/ekm.h"
#include "RawLog.h"
#include "CrashSim.h"
#include "output.h"

using namespace std;

RawLog::RawLog(void)
{
    struct ekm_get_log_request req;
    int ret;

    req.buffer = NULL;
    do {
        if(req.buffer)
            delete [] req.buffer;
        if(sim()->ekm()->ekm_get_log_size(&req.size) < 0)
            xi_panic("can't get buffer log size from %d", 
                     sim()->ekm());
        req.buffer = new char[req.size];
        assert(req.buffer);
        ret = sim()->ekm()->ekm_get_buffer_log(&req);
        // TODO: prevent infinite loop
    } while (ret < 0);

    _size = req.size;
    _log = req.buffer;

    v_out << "RawLog::RawLog(void): get log of size " << _size << endl;

#if 1
    // this resets lsn
    if(sim()->ekm()->ekm_empty_log() < 0)
        xi_panic("can't empty log!");
#endif
}

void RawLog::to_file(const char* file)
{
    int fd;
    if((fd=open(file, O_WRONLY|O_CREAT, 0777)) < 0) {
        perror("open");
        assert(0);
    }

    ::write(fd, &_size, sizeof _size);

    int count = 0, ret;
    while(count < size()) {
        if((ret=::write(fd, log()+count, size()-count)) < 0) {
            perror("write");
            assert(0);
        }
        count += ret;
    }
    close(fd);
}

void RawLog::from_file(const char* file)
{
    int fd;
    if((fd=open(file, O_RDONLY, 0777)) < 0) {
        perror("open");
        assert(0);
    }

    ::read(fd, &_size, sizeof _size);
    _log = new char[_size];

    int count = 0, ret;
    while(count < size()) {
        if((ret=::read(fd, _log+count, size()-count)) < 0) {
            perror("read");
            assert(0);
        }
        count += ret;
    }
    close(fd);
}

RawLog::~RawLog(void)
{
    delete [] _log;
}

