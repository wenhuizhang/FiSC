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


#include <iostream>

#include "crash.h"
#include "FsTestDriver.h"

#include "writes.h"

using namespace std;


//####################################################################
#ifdef CONF_EKM_LOG_SYSCALL
StableFS::StableFS(void)
    :  _c(unordered, INT_MAX, INT_MAX)
{
    ongoing_syscall = NULL;
    _perm = NULL;
    _interests.push_back(EKM_SYSCALL_CAT);
}

void StableFS::start(void)
{
    // FIXME: should construct stable state from disk.  now assume
    // we start from an empty stable state
    assert(fds.empty());
    assert(syscalls.size() == 0);
    assert(ongoing_syscall == NULL);
	
    assert(_volatile_fs._objs.size() == 1);
    assert(_objs.size() == 1);
}

void StableFS::finish(void)
{
    clear();
}

void StableFS::clear(void)
{
    fds.clear();
    clear_syscalls();
    _volatile_fs.clear();
    AbstFS::clear();
    ongoing_syscall = NULL;
}

// TODO: need to permute when we remove syscalls from pending
// syscall list == sync operations
void StableFS::replay(Record *rec)
{
    internal_check();

    SyscallRec *sr = static_cast<SyscallRec*> (rec);

    replay_syscall(sr);

    v_out << "StableFS: replayed syscall record " << *sr << endl;
}

int StableFS::check()
{
    xi_assert(_driver, "StableFS not attached to a FS test driver!");

    internal_check();

    AbstFS recovered_fs(_driver);
    _driver->_ekm->abstract_path
        (_driver->_actual_fs->path(), recovered_fs);
    static int counter = 0;
    counter ++;
    vv_out  <<"\n\n"
            << "---------------------------------------\n"
            << "StableFS: check " << counter << "\n"
            << "StableFS: recovered file system:\n"
            << recovered_fs
            << "StableFS: stable file system:\n"
            << (*this)
            << "\n";

    signature_t recovered_sig = recovered_fs.get_sig_with_name();
    if(_valid_fs_sigs.find(recovered_sig) != _valid_fs_sigs.end()) {
        vv_out <<"StableFS: recovered file system is already validated\n";
        sim_stat.fs_hits ++;
        return CHECK_SUCCEEDED;
    }
    sim_stat.fs_misses ++;

    syscall_list_t::iterator it;
    syscall_list_t::reverse_iterator rit;
    Powerset::set_t subset;
	
    delta_vec_t deltas;
    delta_vec_t data_deltas;
    delta_vec_t::iterator dit;

    for(it=syscalls.begin(); it!=syscalls.end();it++) {
        int i = 0;
        for(i = 0; i < 2; i ++) {
            struct delta *delta= &(*it)->deltas[i];

            if(!delta->valid())
                continue;

            if(delta->type == TRUNCATE || delta->type == PWRITE)
                data_deltas.push_back(delta);
            else
                deltas.push_back(delta);
        }
    }

    if(ongoing_syscall) {
        vv_out << "StableFS: NOTE there is an onging syscall "
               << *ongoing_syscall << endl;
    }

    if(output_vv) {
        cout<<"StableFS: permuting " << deltas.size() <<" metadata deltas\n";
        int i = 0;
        for(i=0, dit=deltas.begin(); dit!=deltas.end(); i++, dit++)
            cout << "StableFS:        "
                 << "metadata " << i << ": " << *(*dit) << endl;
		
        cout<<"\n"
            <<"StableFS: total " << data_deltas.size() <<" data deltas\n";
        for(i=0, dit=data_deltas.begin(); dit!=data_deltas.end(); dit++)
            cout << "StableFS:        "
                 << "data " << i << ": " << *(*dit) << endl;
    }

    bool too_many = deltas.size() > MAX_SET;
    if(too_many) {
        // clear here to save memory.  Our system is structured in
        // a way that even if we don't clear mem here, next time
        // when init_replay is called, mem will be freed
        clear();
        throw Permuter::Abort(__FILE__, __LINE__, 
                              "StableFS: too many pending deltas!");
        // non-reachable
        assert(0);
    }

    // TODO  track dependency between deltas to prune combinations
    int iter = 0;
    bool pruned, valid = false;
    AbstFS fs(_driver);

    _c.init(deltas.size(), unordered, INT_MAX, INT_MAX);
    while(_c.next()) {
        fs.copy(*this);
		
        pruned = false;

        // NOTE: deltas must be applied in syscall order.
        // otherwise, a valid set of deltas may become invalid,
        // due to dependencies between deltas
        const Powerset::set_t& subset = *_c;
        Powerset::set_t::iterator it;
        for(it=subset.begin(); it!=subset.end(); it++) {
            int j = *it;
            if(pruned=!make_delta_stable(fs, deltas[j])) {
                _c.skip_prefix();
                vv_out <<"StableFS: pruned deltas with prefix: "
                       << subset << "\n";
                break;
            }
        }

        if(pruned || fs.detect_dangling()) {
            vv_out<<"StableFS: pruned delta set:" << subset <<"\n";
            continue;
        }

        vv_out<<"\n\n";
        vv_out<<"StableFS: applying delta set " << iter++ <<": "
              << subset << "\n";

#if 0
        cout<<"stable fs + subset of system calls:\n"
            << fs;

        // no need to remove orphans. the canon_* methods take care of it
        fs.remove_orphans();
        cout<<"stable fs + subset of system calls, with orphans removed:\n"
            << fs;
#endif

        // need to make sure ref cnt is correct
        fs.fix_refcnts();

        _valid_fs_sigs[fs.get_sig_with_name()] = true;

        // check metadata first
        if(fs.compare_metadata(recovered_fs)) {
            vv_out <<"StableFS: metadata don't match\n"
                   <<"StableFS: stable fs + subset of system calls, with orphans removed:\n"
                   << fs
                   <<"StableFS: recovered fs:\n"
                   << recovered_fs;
        } else {
            // check if two file systems are identical
            vv_out << "StableFS: metadata identical. now check file data\n";
            if(!fs.compare(recovered_fs)) {
                vv_out << "StableFS: data identical.\n";
                valid = true;
                break;
            }

            // check if the differences between files can be
            // described by the set of pending data deltas
            if(get_option(stablefs, check_metadata_only)) {
                valid = true;
                break;
            }
            valid = check_data(fs, recovered_fs, data_deltas);
            break;
        }
    }

    if(!valid) {
        cout <<"StableFS: check " << counter << " failed: "
             <<"recovered fs can't be covered by stable fs + deltas\n"
             <<"StableFS: stable fs:\n" << fs
             <<"StableFS: recovered fs:\n" << recovered_fs;
		clear(); // cleanup before calling error
        error("stablefs", "stablefs");
        // not reachable
        assert(0);
    }
	
    vv_out <<"StableFS: check " << counter << " succeeded. "
           << "Adding recovered fs to valid fs set.\n";
    /* assert(_valid_fs_sigs.find(recovered_sig) 
       == _valid_fs_sigs.end());  */
    _valid_fs_sigs[recovered_sig] = true;
    return CHECK_SUCCEEDED;
}

void StableFS::update()
{
#if 0
    // can't update stable fs here.  there could be some objects
    // that are stable but we can't see through ekm_abstract_path.
    // i.e., orphans.
    ekm_abstract_path(_driver->_ekm, _driver->_actual_fs->path(),
                      _volatile_fs);
#endif
}

const char* StableFS::delta_names[] = {
    "add",
    "truncate",
    "pwrite",
    "add_entry", "remove_entry", "replace_entry",
};

#define DECL(dir, fn, Fn) \
void StableFS::replay_##dir##_##fn(Enter##Fn##Rec* enter, Exit##Fn##Rec* exit)

DECL(enter, open, Open)
{
    int fd;
    meta *o = NULL;

    if((fd=exit->ret) >= 0) {
        assert(fds.find(fd) == fds.end());
        // FIXME total hack
        if(enter->filename[0] != '/' 
           || _driver->_actual_fs->ignore(enter->filename))
            return;
        // check if the open syscall is actually a creat
        if(_volatile_fs.stat(enter->filename, o) < 0)
            replay_enter_creat(enter, exit);
    }
}

DECL(exit, open, Open)
{
    Sgi::hash_map<int, struct file_des>::iterator fds_it;
    int fd;
    meta *o = NULL;

    if((fd=exit->ret) >= 0) {
        assert(fds.find(fd) == fds.end());
        cout << " open returns fd = " << fd << endl;
        if(enter->filename[0] != '/' 
           || _driver->_actual_fs->ignore(enter->filename))
            return;
        // FIXME total hack
        fds[fd].filename = enter->filename;
        fds[fd].pos = 0;
        fds[fd].osync_p = enter->flags & O_SYNC;

        // check if the open syscall is actually a creat
        if(_volatile_fs.stat(enter->filename, o) < 0)
            replay_exit_creat(enter, exit);
    }
}

DECL(enter, close, Close)
{
    // do nothing
}

DECL(exit, close, Close)
{
    Sgi::hash_map<int, struct file_des>::iterator fds_it;
    int fd;

    //cout << "close returns " << exit->ret << endl;
    if(exit->ret >=0) {
        fd = enter->fd;

        // can have a close logged, but the corresponding open
        // isn't logged, probably because we started tracing after
        // we opened the directory /mnt/sbd0

        // assert((fds_it=fds.find(fd)) != fds.end());
        if((fds_it = fds.find(fd)) != fds.end())
            fds.erase(fds_it);
        //cout << " close fd = " << fd << endl;
    }
}

DECL(enter, creat, Open)
{
    int ret;

    if(exit->ret < 0) 
        return;

    ret = _volatile_fs.creat(enter->filename, enter->mode);
    assert(ret >= 0);

    namei_t nd = namei_init;
    struct delta *delta;
    int child_id;
    struct syscall *call = new struct syscall(enter, exit);

    // delta to the file
    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->filename, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->filename);
    delta->add(nd.obj->id, ABST_FILE, enter->mode);

    // delta to the parent directory
    delta = &call->deltas[1];
    nd = parent_namei_init;				
    ret = _volatile_fs.walk_path(enter->filename, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->filename);
    child_id = CAST(dir*, nd.obj)->get_entry(nd.name);
    assert(child_id >= 0);
    delta->add_entry(nd.obj->id, nd.name, child_id);

    syscalls.push_back(call);
}

DECL(exit, creat, Open)
{
    if(exit->ret < 0) 
        return;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);
	
    if(_driver->actual_fs()->mount_sync_p) {
        syscalls.pop_back();
        delete call;
        creat(enter->filename, enter->mode);
        on_sync_ops();
        return;
    }

    // delta to the file
    if(enter->flags & O_SYNC) {
        bool applied = make_delta_stable(*this, &call->deltas[0]);
        assert(applied);
        call->clear_delta(0);
        on_sync_ops();
    }
}

DECL(enter, ftruncate, Ftruncate)
{
    int ret, fd;

    if(exit->ret <0)
        return;

    fd = enter->fd;
    assert(fds.find(fd) != fds.end());
    ret = _volatile_fs.truncate(fds[fd].filename, 
                                enter->length);
    assert(ret >= 0);

    namei_t nd = namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);

    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(fds[fd].filename, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   fds[fd].filename);
    delta->truncate(nd.obj->id, enter->length);
    
    syscalls.push_back(call);
}

DECL(exit, ftruncate, Ftruncate)
{
    int fd;

    if(exit->ret < 0)
        return;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    fd = enter->fd;
    assert(fds.find(fd) != fds.end());

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        truncate(fds[fd].filename, 
                 enter->length);
        on_sync_ops();
        return;
    }

    if(fds[fd].osync_p) {
        bool applied = make_delta_stable(*this, &call->deltas[0]);
        assert(applied);
        call->clear_delta(0);
        on_sync_ops();
    }
}

DECL(enter, pwrite64, Pwrite64)
{
    int ret, fd;
    const char* filename;

    if(exit->ret < 0)
        return;
	
    fd = enter->fd;
    assert(fds.find(fd) != fds.end());
    filename = fds[fd].filename;
    ret = _volatile_fs.pwrite(filename, enter->buf, enter->count, enter->pos);
    assert(ret >= 0);
	
    namei_t nd = namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);

    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(fds[fd].filename, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   fds[fd].filename);
    delta->pwrite(nd.obj->id, enter->buf, enter->count, enter->pos);
	
    syscalls.push_back(call);
}

DECL(exit, pwrite64, Pwrite64)
{
    int fd;
    const char* filename;

    if(exit->ret < 0)
        return;
	
    fd = enter->fd;
    assert(fds.find(fd) != fds.end());
    filename = fds[fd].filename;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        pwrite(filename, enter->buf, enter->count, enter->pos);
        on_sync_ops();
        return;
    }
	
    if (fds[fd].osync_p) {
        bool applied = make_delta_stable(*this, &call->deltas[0]);
        assert(applied);
        call->clear_delta(0);
        on_sync_ops();
    }
}

DECL(enter, mkdir, Mkdir)
{
    int ret;

    if(exit->ret < 0)
        return;

    ret = _volatile_fs.mkdir(enter->pathname, enter->mode);
    assert(ret >= 0);

    namei_t nd = namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);
    int child_id;

    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->pathname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->pathname);
    delta->add(nd.obj->id, ABST_DIR, enter->mode);

    delta = &call->deltas[1];
    nd = parent_namei_init;				
    ret = _volatile_fs.walk_path(enter->pathname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->pathname);
    child_id = CAST(dir*, nd.obj)->get_entry(nd.name);
    assert(child_id >= 0);
    delta->add_entry(nd.obj->id, nd.name, child_id);

    syscalls.push_back(call);
}

DECL(exit, mkdir, Mkdir)
{
    if(exit->ret < 0)
        return;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        mkdir(enter->pathname, enter->mode);
        on_sync_ops();
        return;
    }
}

DECL(enter, unlink, Unlink)

{
    int ret;

    if(exit->ret < 0)
        return;
	
    namei_t nd_unlink = namei_init;
    ret = _volatile_fs.walk_path(enter->pathname, nd_unlink);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->pathname);

    // copy id because we're going to delete nd_unlink.obj
    int target_id = nd_unlink.obj->id;
    ret = _volatile_fs.unlink(enter->pathname);
    assert(ret >= 0);
	
    namei_t nd = parent_namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);

    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->pathname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->pathname);
	
    delta->remove_entry(nd.obj->id, nd.name, target_id);

    syscalls.push_back(call);
}

DECL(exit, unlink, Unlink)
{
    if(exit->ret < 0)
        return;
	
    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        unlink(enter->pathname);
        on_sync_ops();
        return;
    }
}

DECL(enter, rmdir, Rmdir)
{
    int ret;

    if(exit->ret < 0)
        return;

    namei_t nd_rmdir = namei_init;
    ret = _volatile_fs.walk_path(enter->pathname, nd_rmdir);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->pathname);

    // copy id because we're going to delete nd_rmdir.obj
    int target_id = nd_rmdir.obj->id;
    ret = _volatile_fs.rmdir(enter->pathname);
    assert(ret >= 0);

    namei_t nd = parent_namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);

    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->pathname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->pathname);
    delta->remove_entry(nd.obj->id, nd.name, target_id);

    syscalls.push_back(call);
}

DECL(exit, rmdir, Rmdir)
{
    if(exit->ret < 0)
        return;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        rmdir(enter->pathname);
        on_sync_ops();
        return;
    }
}

DECL(enter, link, Link)
{
    int ret;

    if(exit->ret < 0)
        return;

    ret = _volatile_fs.link(enter->oldname, enter->newname);
    assert(ret >= 0);

    namei_t nd = parent_namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);
    int child_id;

    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->newname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->newname);
    child_id = CAST(dir*, nd.obj)->get_entry(nd.name);
    assert(child_id >= 0);
    delta->add_entry(nd.obj->id, nd.name, child_id);

    syscalls.push_back(call);
}

DECL(exit, link, Link)
{
    if(exit->ret < 0)
        return;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        link(enter->oldname, enter->newname);
        on_sync_ops();
        return;
    }
}

DECL(enter, rename, Rename)
{
    int ret;

    if(exit->ret < 0)
        return;

    namei_t nd_rename = namei_init;
    ret = _volatile_fs.walk_path(enter->newname, nd_rename);
#if 0
    // could fail, if newname doesn't exist
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->newname);
#endif
    int old_child_id;
    if(nd_rename.obj)
        old_child_id = nd_rename.obj->id;
    else
        old_child_id = INVALID_ID;

    int child_id;
    namei_t nd_rename_oldpath = namei_init;
    ret = _volatile_fs.walk_path(enter->oldname, nd_rename_oldpath);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->oldname);
    child_id = nd_rename_oldpath.obj->id;

    ret = _volatile_fs.rename(enter->oldname, enter->newname);
    assert(ret >= 0);

    namei_t nd = parent_namei_init;				
    struct delta *delta;
    struct syscall *call = new struct syscall(enter, exit);
    int new_child_id;

    // link replacement must be atomic. so one delta
    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->newname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->newname);
    new_child_id = CAST(dir*, nd.obj)->get_entry(nd.name);

    if(old_child_id != INVALID_ID)
        delta->replace_entry(nd.obj->id, nd.name, 
                             old_child_id, new_child_id);
    else
        delta->add_entry(nd.obj->id, nd.name, new_child_id);
	

    delta = &call->deltas[1];
    ret = _volatile_fs.walk_path(enter->oldname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->oldname);
    delta->remove_entry(nd.obj->id, nd.name, child_id);
	
    syscalls.push_back(call);
}

DECL(exit, rename, Rename)
{
    if(exit->ret < 0)
        return;

    assert(syscalls.size() > 0);
    struct syscall *call = syscalls.back();
    assert(call->enter == enter && call->exit == exit);

    if(_driver->actual_fs()->mount_sync_p) {
        call->clear_deltas();
        rename(enter->oldname, 
               enter->newname);
        on_sync_ops();
        return;
    }
}

#if 0
void StableFS::replay_symlink(struct ekm_syscall_record *enter,
                              struct ekm_syscall_record *exit)
{
    int ret;
	
    if(exit->u.sys_symlink_res.ret < 0)
        return;

    ret = _volatile_fs.symlink(enter->u.sys_symlink.oldname, 
                               enter->u.sys_symlink.newname);
    assert(ret >= 0);
    if(_driver->actual_fs()->mount_sync_p) {
        symlink(enter->u.sys_symlink.oldname, 
                enter->u.sys_symlink.newname);
        return;
    }
		
    namei_t nd = namei_init;
    struct delta *delta;
    int child_id;
    struct syscall *call = new struct syscall(enter, exit);

    // not done yet
    delta = &call->deltas[0];
    ret = _volatile_fs.walk_path(enter->u.sys_symlink.newname, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", 
                   enter->u.sys_symlink.newname);

    nd = parent_namei_init;				

    syscalls.push_back(call);
}
#endif

DECL(enter, sync, Sync)
{
    // do nothing
}

DECL(exit, sync, Sync)
{
    // flush all pending operations
    clear_syscalls();
    // set stable fs to be volatile fs
    copy(_volatile_fs);
    on_sync_ops();
}

DECL(enter, fsync, Fsync)
{
    // do nothing
}


DECL(exit, fsync, Fsync)
{
    int fd;

    if(exit->ret < 0)
        return;
	
    if(_driver->actual_fs()->mount_sync_p)
        return;

    meta *o;
    fd = enter->fd;
    assert(fds.find(fd) != fds.end());

#if 0
    // file may be open not osync. still need to flush
    if(fds[fd].osync) // should already synced
        return;
#endif

    o = make_obj_stable(fds[fd].filename, true);

    syscall_list_t::iterator it, next_it;
    next_it = syscalls.begin();
    while(next_it != syscalls.end()) {
        it = next_it++;

        // remove deltas for this file
        struct syscall *call = (*it);
        if(call->deltas[0].target_id == (int) o->id)
            call->deltas[0].clear();
        if(call->deltas[1].target_id == (int) o->id)
            call->deltas[1].clear();
		
        // if both deltas are invalid, remove this syscall
        if(!call->deltas[0].valid() && !call->deltas[1].valid()) {
            syscalls.erase(it);
            delete call;
        }
    }
    on_sync_ops();
}

#undef DECL

// FIXME: when permute to generate possible stable FS, need to
// consider ongoing syscalls too
void StableFS::replay_syscall(SyscallRec *sr)
{
    syscall_list_t::iterator it;
    Sgi::hash_map<int, struct file_des>::iterator fds_it;
    struct ekm_syscall_record *exit;

    _last_op_is_sync = 0;
    switch(sr->type()) {
    case EKM_enter_fdatasync:
    case EKM_exit_fdatasync:
    case EKM_enter_llseek:
    case EKM_exit_llseek:
    case EKM_enter_read:
    case EKM_exit_read:
    case EKM_enter_write:
    case EKM_exit_write:
    case EKM_enter_symlink:
    case EKM_exit_symlink:
    case EKM_enter_lseek:
    case EKM_exit_lseek:
        cout << "system call " << LogMgr::the()->name(sr->type())
             << "not handled " << endl;
        break;

#define CASE_ENTER(fn, Fn)                             \
    case EKM_enter_##fn: {                             \
        assert(ongoing_syscall == NULL);               \
        Enter##Fn##Rec *enter;                         \
        Exit##Fn##Rec *exit;                           \
        enter = static_cast<Enter##Fn##Rec*>(sr);      \
        exit = enter->other_half;                      \
        replay_enter_##fn(enter, exit);                \
        ongoing_syscall = sr;                          \
        break;                                         \
    }

        CASE_ENTER(open, Open);
        CASE_ENTER(ftruncate, Ftruncate);
        CASE_ENTER(pwrite64, Pwrite64);
        CASE_ENTER(mkdir, Mkdir);
        CASE_ENTER(rename, Rename);
        CASE_ENTER(rmdir, Rmdir);
        CASE_ENTER(unlink, Unlink);
        CASE_ENTER(link, Link);
        CASE_ENTER(close, Close);
        CASE_ENTER(fsync, Fsync);
        CASE_ENTER(sync, Sync);

#undef CASE_ENTER

#define CASE_EXIT(fn, Fn)                             \
    case EKM_exit_##fn: {                             \
        assert(ongoing_syscall != NULL);              \
        Enter##Fn##Rec *enter;                        \
        Exit##Fn##Rec *exit;                          \
        exit = static_cast<Exit##Fn##Rec*>(sr);       \
        enter = exit->other_half;                     \
        assert(ongoing_syscall == enter);             \
        replay_exit_##fn(enter, exit);                \
	ongoing_syscall = NULL;                       \
	break;                                        \
    }

        CASE_EXIT(open, Open);
        CASE_EXIT(close, Close);
        CASE_EXIT(sync, Sync);
        CASE_EXIT(fsync, Fsync);
        CASE_EXIT(ftruncate, Ftruncate);
        CASE_EXIT(pwrite64, Pwrite64);
        CASE_EXIT(mkdir, Mkdir);
        CASE_EXIT(rename, Rename);
        CASE_EXIT(rmdir, Rmdir);
        CASE_EXIT(unlink, Unlink);
        CASE_EXIT(link, Link);

#undef CASE_EXIT

    default: 
        cout << "system call " << LogMgr::the()->name(sr->type())
             << "not handled " << endl;
        assert(0);
    }
	
}

AbstFS::meta* StableFS::find_volatile_obj(const char* path_in_volatile)
{
    int ret;
    meta *o;
    ret = _volatile_fs.stat(path_in_volatile, o);
    assert(ret >= 0);
    return o;
}

AbstFS::meta* StableFS::make_obj_stable(const char* path_in_volatile, 
                                        bool create_p)
{
    meta *vo, *o;
    vo = find_volatile_obj(path_in_volatile);

    int id = vo->id;
    if(_objs.find(id) != _objs.end())
        _objs[id]->copy(vo);
    else if(create_p) {
        o = vo->clone();
        add_new_obj(o, vo->id);
    }
    else
        assert(0);

    return _objs[id];
}

bool StableFS::make_delta_stable(AbstFS &fs, struct delta *delta)
{
    SyscallRec* enter;
    int id, delta_nr, old_child_id;
    file *f;
    dir *d;
    meta *o;

    assert(delta && delta->call && delta->call->enter);
    enter = delta->call->enter;
    delta_nr = delta->delta_nr;
	
    if(enter->type() == EKM_enter_open
       || enter->type() == EKM_enter_mkdir
       || enter->type() == EKM_enter_rename)
        assert(delta_nr == 0 || delta_nr == 1);
    else
        assert(delta_nr == 0);

    id = delta->target_id;
    assert(id >= 0);

    switch(delta->type) {
    case ADD:
        assert(fs._objs.find(delta->target_id) == fs._objs.end());
        o = new_obj(delta->u.add.type, delta->u.add.mode);
        assert(delta->target_id > 0);
        fs.add_new_obj(o, delta->target_id);
        break;
	
    case TRUNCATE:
        if(fs._objs.find(delta->target_id) == fs._objs.end())
            return false;
        assert(fs._objs[id]->type == ABST_FILE);
        f = CAST(file*, fs._objs[id]);
        f->truncate(delta->u.truncate.length);
        break;

    case PWRITE:
        if(fs._objs.find(delta->target_id) == fs._objs.end())
            return false;
        assert(fs._objs[id]->type == ABST_FILE);
        f = CAST(file*, fs._objs[id]);
        f->pwrite(delta->u.pwrite.buf, delta->u.pwrite.size, 
                  delta->u.pwrite.off);
        break;
	
    case ADD_ENTRY:
        if(fs._objs.find(id) == fs._objs.end())
            return false; // prune this delta set
        assert(fs._objs[id]->type == ABST_DIR);
        d = CAST(dir*, fs._objs[id]);
        if(d->get_entry(delta->u.entry.name) != -1)
            return false;

        fs.link_obj(d, delta->u.entry.child_id, delta->u.entry.name);
        break;

    case REMOVE_ENTRY:
        if(fs._objs.find(id) == fs._objs.end())
            return false; // prune this delta set
		
        assert(fs._objs[id]->type == ABST_DIR);
        d = CAST(dir*, fs._objs[id]);
        if(d->get_entry(delta->u.entry.name)
           != delta->u.entry.child_id)
            return false;
		
        fs.unlink_obj(d, delta->u.entry.name);
        break;

    case REPLACE_ENTRY:
        if(fs._objs.find(id) == fs._objs.end())
            return false; // prune this delta set

        assert(fs._objs[id]->type == ABST_DIR);
        d = CAST(dir*, fs._objs[id]);
        old_child_id = d->get_entry(delta->u.replace_entry.name);
        if(old_child_id == INVALID_ID
           || old_child_id != delta->u.replace_entry.old_child_id)
            return false;
		
        fs.unlink_obj(d, delta->u.replace_entry.name);
        fs.link_obj(d,delta->u.replace_entry.new_child_id, 
                    delta->u.replace_entry.name);
        break;

    default:
        assert(0);
    }
    return true;
}

void StableFS::delta::add(int target_id, 
                          abst_file_type_t target_type, 
                          mode_t target_mode) 
{
    this->type = ADD;
    this->target_id = target_id;
    u.add.type = target_type;
    u.add.mode = target_mode;
}

void StableFS::delta::truncate(int target_id, off_t length)
{
    this->target_id = target_id;
    this->type = TRUNCATE;
    u.truncate.length = length;
}

void StableFS::delta::pwrite(int target_id, const void* buf, 
                             size_t size, off_t off)
{
    this->target_id = target_id;
    this->type = PWRITE;
    u.pwrite.buf = buf;
    u.pwrite.size = size;
    u.pwrite.off = off;
}

void StableFS::delta::add_entry(int target_id, const char* name, 
                                int child_id)
{
    this->target_id = target_id;
    this->type = ADD_ENTRY;
    u.entry.name = xstrdup(name);
    u.entry.child_id = child_id;
}

void StableFS::delta::remove_entry(int target_id, const char* name,
                                   int child_id)
{
    this->target_id = target_id;
    this->type = REMOVE_ENTRY;
    u.entry.name = xstrdup(name);
    u.entry.child_id = child_id;
}

void StableFS::delta::replace_entry(int target_id, const char* name, 
                                    int old_child_id, int new_child_id)
{
    this->target_id = target_id;
    this->type = REPLACE_ENTRY;
    u.replace_entry.name = xstrdup(name);
    u.replace_entry.old_child_id = old_child_id;
    u.replace_entry.new_child_id = new_child_id;
}

void StableFS::delta::print(ostream& o) const
{
    o << delta_names[type] << "(" << target_id;
    switch(type) {
    case ADD:
        o << ", " << AbstFS::type_char[u.add.type]
          << ", " << u.add.mode;
        break;
    case TRUNCATE:
        o << ", " << u.truncate.length;
        break;
    case PWRITE:
        o << ", " << u.pwrite.size 
          << ", " << buf_to_hex((const char*)u.pwrite.buf, u.pwrite.size)
          << ", " << u.pwrite.off;
        break;
    case ADD_ENTRY:
    case REMOVE_ENTRY:
        o << ", " << u.entry.name
          << ", " << u.entry.child_id;
        break;
    case REPLACE_ENTRY:
        o << ", " << u.replace_entry.name
          << ", " << u.replace_entry.old_child_id
          << ", " << u.replace_entry.new_child_id;
        break;
    default:
        assert(0);
    }
    o << ") ";
    o << LogMgr::the()->name(call->enter->type())
      << ", lsn=" << call->enter->lsn()
      << ", delta_nr=" << delta_nr;
}

ostream& operator<<(ostream& o, const StableFS::delta& d)
{
    d.print(o);
    return o;
}

#if 0
// TODO: need to clear this and _volatile_fs somewhere
int StableFS::mount(const char *target, 
                    unsigned long mountflags, const void *data)
{
    AbstFS::mount(target, mountflags, data);
    _volatile_fs.mount(target, mountflags, data);
    _volatile_fs.set_driver(_driver);
    return 0;
}
#endif

void StableFS::mount_path(const char *mnt)

{
    AbstFS::mount_path(mnt);
    _volatile_fs.mount_path(mnt);
}

void StableFS::driver(FsTestDriverBase *driver)
{
    AbstFS::driver(driver);
    _volatile_fs.driver(driver);
}

struct StableFS::shorter {
    off_t size;
    shorter(off_t size) {this->size = size;}

    bool operator()(struct delta * d) {
        return d->type == TRUNCATE 
            && d->u.truncate.length <= size;
    }
};

struct StableFS::longer {
    off_t size;
    longer(off_t size) {this->size = size;}

    bool operator()(struct delta * d) {
        return (d->type == TRUNCATE 
                && d->u.truncate.length >= size)
            || (d->type == PWRITE 
                && (off_t)(d->u.pwrite.off + d->u.pwrite.size) >= size);
    }
};

struct StableFS::equal_byte {
    char c;
    off_t off;

    equal_byte(char c, off_t off) {this->c = c; this->off = off;}

    bool operator()(struct delta *d) {
        return (d->type == PWRITE
                && off >= d->u.pwrite.off
                && off < (off_t)(d->u.pwrite.off + d->u.pwrite.size)
                && c == ((const char*)d->u.pwrite.buf)[off-d->u.pwrite.off]);
    }
};

class EndVisit {
};

class StableFS::diff_region: public region_visitor {
private:
    off_t _file_size;
    delta_list_t *_deltas;
    data* _other_d;
    bool _diff_to_zero;
public:
    set<struct delta*> _result;
    diff_region(data* other_d, off_t size, 
                delta_list_t *deltas, bool d_z) {
        _other_d = other_d;
        _file_size = size;
        _deltas = deltas;
        _diff_to_zero = d_z;
    }
    void operator()(struct data_region *dr);
};

// maybe shouldn't use the same code to compare two things?  when
// _diff_to_zero is set, _other_d is the recovered data, the data
// obj we're visiting is the stable data.  when _diff_to_zero is
// not set, _other_d is the stable data and the data obj we're
// visiting is the recovered data.
void StableFS::diff_region::operator()(struct data_region *dr) 
{
    signature_t v1, v2;
    off_t off;
    int i;
    delta_list_t::iterator it;

    for(off=dr->start; off<dr->end && off<_file_size; off+=sizeof v1) {

        // FIXME: tricky case when read beyond _file_size
        if(dr->magic)
            v2 = position_hash(off, dr->magic);
        else
            memcpy(&v2, dr->data + off-dr->start, sizeof v2);

        _other_d->pread(&v1, sizeof v1, off);

        char c1, c2;
        for(i = 0; i < (int)sizeof v1; i++) {
            c1 = ((char*)&v1)[i];
            c2 = ((char*)&v2)[i];

            if(!_diff_to_zero) {
                // if our value (c2, recovered) isn't equal to their
                // value(c1, stable), find a write that can explain our
                // value(c2, recovered)
                if(c1 == c2) continue;

                it = find_if(_deltas->begin(), _deltas->end(), 
                             equal_byte(c2, off+i) );
                if(it == _deltas->end()) {
                    cout<<"StableFS: byte " << (int)c2 << " at offset " 
                        << off+i << " in recovered file is never written\n";
					
                    // value (c2, recovere) comes from 0.  can happen
                    // when there is a dangling pointer in inode or
                    // indirect blocks that points to garbage blocks,
                    // which can happen at least in ext2.  however, if
                    // we are only interested in data loss, we can
                    // ignore this case.
                    if(get_option(stablefs, check_data_loss_only)
                       && c1 == 0) {
                        cout << "StableFS: check_data_loss_only is set. continue\n";
                        continue;
                    }

                    throw EndVisit();
                    // it's possible that other subset of deltas
                    // can explain this recovered fs.
                    //error("write", "value %d at offset %d can't be described by deltas", (int)c2, (int)(off+i));
                }
            } else {
                // already checked if their value is non-zero,
                if(c1 != 0) continue; 
                // our value (c2, stable) becomes zero (c1,
                // recovered). find a write that zeroes our value
                it = find_if(_deltas->begin(), _deltas->end(), 
                             equal_byte(0, off+i) );
                if(it == _deltas->end()) {
                    cout<<"StableFS: byte " << (int)c2 << "at offset " 
                        << off+i << " in stable file is never zeroed\n";
                    // it's possible that other subset of deltas
                    // can explain this recovered fs.
                    //error("write", "value %d at offset %d can't be described by deltas", (int)c2, (int)(off+i));
                    throw EndVisit();
                }
            }
            _result.insert((*it));
        }
    }
}

// check_data is called only when metadata are identical.
// check if differences between stable and recovered can be
// described by the set of data deltas
bool StableFS::check_data(AbstFS& stable, AbstFS &recovered,
                          delta_vec_t& all_file_deltas)
{
    // algorithm:
    // find files that diffs in fs and recovered fs
    // for each such file
    //		check size
    //		check data

    //fs1 is stable fs.  fs2 is recovered fs;
    AbstFS &fs1 = stable;
    AbstFS &fs2 = recovered;

    id_map_t id_map1, id_map2;
    vector<int> order1, order2;
    delta_list_t file_deltas;
    delta_list_t::iterator it;

    fs1.dfs_order(id_map1, true);
    id_map_to_order(id_map1, order1);

    fs2.dfs_order(id_map2, true);
    id_map_to_order(id_map2, order2);

    // must be true since metadata identical
    assert(order1.size() == order2.size());

    int i;
    for(i = 0; i < (int)order1.size(); i++) {
        int id1 = order1[i];
        int id2 = order2[i];

        assert(fs1._objs.find(id1) != fs1._objs.end());
        meta *o1 = fs1._objs[id1];
        assert(fs2._objs.find(id2) != fs2._objs.end());
        meta *o2 = fs2._objs[id2];

        // must be true since metadata identical
        assert(o1->type == o2->type);

        if(!is_file(o1)) continue;
        if(!compare_obj(o1, o2)) continue;

        cout <<"StableFS: file in stable:\n";
        print_obj(o1, NULL, cout);
        cout <<"StableFS: file in recovered:\n";
        print_obj(o2, NULL, cout);

        file *f1, *f2;
        f1 = CAST(file*, o1);
        f2 = CAST(file*, o2);

        // find all deltas related to fhis object
        find_obj_deltas(f1->id, all_file_deltas, file_deltas);
		
        // check if size diff can be described
        if(f1->size  > f2->size) {
            // if become smaller, must have a truncate <= size in recovered
            it = find_if(file_deltas.begin(), file_deltas.end(), 
                         shorter(f2->size));
            if(it == file_deltas.end()) {
                cout <<"StableFS: file size diff can't be dscribed by deltas\n"; 
                /* error("stablesize", "file size can't be described by "\
                   "deltas. file stable size %d, recovered size %d", 
                   f1->size, f2->size); */
                return false;
            } else
                cout << "StableFS: size diff is covered by: \n" 
                     << "StableFS:        " << *(*it) << "\n";
        }
        else if (f1->size < f2->size) {
            // if become bigger, must have a truncate or pwrite >=
            // size in recover
            it = find_if(file_deltas.begin(), file_deltas.end(), 
                         longer(f2->size));
            if(it == file_deltas.end()) {
                cout <<"StableFS: file size diff can't be dscribed by deltas\n"; 
                // file becomes bigger, it may be okay if we don't
                // lose old data
                if(!get_option(stablefs, check_data_loss_only))
                    return false;
                cout << "StableFS: check_data_loss_only is set. continue\n";
                /* error("stablesize", "file size can't be described by "\
                   "deltas. file stable size %d, recovered size %d", 
                   f1->size, f2->size); */
            } else
                cout << "StableFS: size diff is covered by: \n" 
                     << "StableFS:        " << *(*it) << "\n";
        } else
            cout << "StableFS: sizes are equal\n";

        // check if data diff can be described.
        set<struct delta*>::iterator sit;

        // for each non-zero byte at some offset, get its value in
        // o2, then check to see if it has the same value in o1.
        // consider the absence of the byte as value 0.  if the
        // byte has a different value in o1, try to find a pwrite
        // delta that writes this value at this offset.  if not,
        // error.
        diff_region diff(&f1->d, f2->size, &file_deltas, false);
		
        try {
            f2->d.for_each_region(&diff);
        } catch (EndVisit e) {
            return false;
        }
		
        if(diff._result.size()) {
            cout << "StableFS: data diff is covered by:\n";
            for(sit=diff._result.begin(); sit!=diff._result.end(); sit++)
                cout << "StableFS:        " << *(*sit) << "\n";
        }

        // for each non-zero byte that only appears in o1 but not
        // o2, check to see if any pwrite delta writes zero to
        // this byte
        diff_region zero(&f2->d, f1->size, &file_deltas, true);

        try {
            f1->d.for_each_region(&zero);
        } catch (EndVisit e) {
            return false;
        }

        if(zero._result.size()) {
            cout << "StableFS: zeroing is covered by:\n";
            for(sit=zero._result.begin(); sit!=zero._result.end(); sit++)
                cout << "StableFS:        " << *(*sit) << "\n";
        }
    }

    cout << "StableFS: all data diff can be described\n";
    return true;
}


void StableFS::clear_syscalls(void)
{
    syscall_list_t::iterator it;
    for(it=syscalls.begin(); it!=syscalls.end(); it++) {
        delete (*it);
    }
    syscalls.clear();
}

void StableFS::internal_check(void)
{
    int i;
    syscall_list_t::iterator it;

    // check syscall list
    for(it=syscalls.begin(); it!=syscalls.end(); it++) {
        struct syscall *call = (*it);

        // pointers ain't corrupted
        for(i = 0; i < 2; i ++)
            assert(call->deltas[i].call == call
                   && call->deltas[i].delta_nr == i);

        // two deltas should be on two objects (or one obj, one
        // -1) if the syscall is not rename
        if(call->enter->type() != EKM_enter_rename)
            assert(call->deltas[0].target_id != call->deltas[1].target_id);

        // if two objects, then the type must be open, mkdir, rename
        if(call->deltas[0].target_id >= 0
           && call->deltas[1].target_id >= 0) {
            assert(call->deltas[0].type != call->deltas[1].type);
            SyscallRec* enter = call->enter;
            assert(enter->type() == EKM_enter_open
                   || enter->type() == EKM_enter_mkdir
                   || enter->type() == EKM_enter_rename);
        }
    }
		
}

void StableFS::find_obj_deltas(int id, delta_vec_t& deltas,
                               delta_list_t& file_deltas)
{
    delta_vec_t::iterator it;

    file_deltas.clear();
    for(it = deltas.begin(); it != deltas.end(); it++) {
        struct delta *d = (*it);
        if(d->target_id == id)
            file_deltas.push_back(d);
    }
}


void StableFS::on_sync_ops()
{
    _last_op_is_sync = 1;
    // invalid valid_fs_sigs
    cout << "StableFS: last op is a sync op.  clear valid fs set\n";
    _valid_fs_sigs.clear();
}

SyscallPermuter* StableFS::permuter(void)
{
    if(!_perm) {
        _perm = dynamic_cast<SyscallPermuter*>(::permuter());
        xi_assert(_perm, "permuer() is not SyscallPermuter!");
    }

    return _perm;
}
#endif
