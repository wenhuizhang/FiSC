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


#include "CheckingContext.h"
#include "CrashSim.h"

CheckingContext::CheckingContext(BlockDevDataVec& init_disks)
    : _init_disks(init_disks)
{
    sim()->umount();
    _raw_log = sim()->stop_trace();
    _log = LogMgr::the()->parse(_raw_log.get());

    // TODO: get final disks
}

CheckingContext::~CheckingContext(void)
{
    delete _log;

    // TODO:
    // free final disks
}
