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


#include "Permuter.h"

class SyscallPermuter;
class SyscallReplayer: public Replayer {
public:
    virtual void replay(Record* rec);

    SyscallReplayer(SyscallPermuter *perm) : _perm(perm)
    { _interests.push_back(EKM_SYSCALL_CAT); }

protected:
    SyscallPermuter *_perm;
};

class SyscallPermuter: public Permuter {
public:
    SyscallPermuter(const char* name)
        : Permuter(name), _syscall_replayer(this)
    { register_replayer(&_syscall_replayer); }

    void check_one_crash(Record* rec);

protected:
    virtual void check_crashes(void);

    SyscallReplayer _syscall_replayer;
};


