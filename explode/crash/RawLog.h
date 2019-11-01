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


#ifndef __RAWLOG_H
#define __RAWLOG_H

#include <boost/shared_ptr.hpp>
class RawLog {

 protected:
        long _size; // size of the log in bytes
        char* _log;

 public:
        RawLog(void);
        ~RawLog(void);

        const char* log(void) const {return _log;}
        long size(void) const {return _size;}
        void to_file(const char* file);
        void from_file(const char* file);
};

typedef boost::shared_ptr<RawLog> RawLogPtr;

#endif // #ifndef __RAWLOG_H
