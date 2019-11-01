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


#include "SyscallPermuter.h"

void SyscallReplayer::replay(Record *rec)
{
	// do nothing if crash checking isn't enabled
	if(!_perm->crash_enabled())
		return;

    // generate a crash only when rec is a modifying file system call
    switch(rec->type()) {
    case EKM_exit_unlink:
    case EKM_exit_mkdir:
    case EKM_exit_rmdir:    
    case EKM_exit_link:
    case EKM_exit_rename:
    case EKM_exit_write:
    case EKM_exit_truncate:
    case EKM_exit_ftruncate:
        return _perm->check_one_crash(rec);
    }
}

void SyscallPermuter::check_crashes(void)
{
    replay(_ctx->log());
}


// generate and check a soft crash
void SyscallPermuter::check_one_crash(Record *end)
{
    // get current working dir
    static char cwd[MAX_PATH];
    getcwd(cwd, sizeof cwd);

    _ctx->init_disks().set_dev_data();
    sim()->mount_kern();
    
    fd_map_t open_fds;
    Log::iterator it;
    
    for_all_iter(it, *_ctx->log()) {
        if((*it)->category() != EKM_SYSCALL_CAT)
            continue;

        // replay system call
        SyscallRec *sys = (SyscallRec*)*it;
        sys->replay(open_fds);
        
        if(sys == end)
            break;

    }

    // close all open files
    fd_map_t::iterator fit;
    for_all_iter(fit, open_fds)
        close((*fit).second);

    // restore working dir
    chdir(cwd);
    sim()->recover_user();
    // call user's checker to check that the recovered storage
    // system is valid
    (*_checker)();
    sim()->umount();

    // can we check crashes during recovery here?
}
