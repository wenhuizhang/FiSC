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


#ifndef __STABLEFS_H
#define __STABLEFS_H

#include <list>
#include <string>

#include <assert.h>

#include "AbstFS.h"
#include "Replayer.h"
#include "Powerset.h"
#include "Combination.h"
#include "driver-options.h"

#ifdef CONF_EKM_LOG_SYSCALL

class SyscallPermuter;

class StableFS: public AbstFS, public Replayer
{
public:

    struct file_des {
        const char* filename;
        bool osync_p;
        off_t pos;
    };

    // FIXME: there is a subtle difference between file
    // creation and directory creation.  file creation has two
    // deltas, the creation delta and the add_entry delta.
    // it's possible that the creation delta is stable, but
    // the add_entry delta isn't.  later, a link operation
    // makes the file visible.  For directory, there is no
    // link operation.  mkdir should have only 1 delta.
    //
    // this is wrong. mkdir shouldn't have only 1 delta: a
    // newly created dir can be renamed
    enum delta_type {
        ADD, /*REMOVE, not needed */  // creat needs two deltas
        TRUNCATE, // for files
        PWRITE, // for files
		//SYM_LINK_UPDATE, // for symlinks
        ADD_ENTRY, REMOVE_ENTRY, REPLACE_ENTRY, // for directories
        DELTA_TYPE_MAX
    };

    struct syscall;
    // can refer to anything in syscall records since they
    // live longer than we do.
    struct delta {
        enum delta_type type;
        int target_id;
        union {
            struct {
                abst_file_type_t type;
                mode_t mode;
            } add;

            struct {
                off_t length;
            } truncate;

            struct {
                const void* buf;
                size_t size;
                off_t off;
            } pwrite;

            struct {
                const char* name;
                int child_id;
            } entry;

            struct {
                const char* name;
                int old_child_id;
                int new_child_id;
            } replace_entry;
        } u;

        struct syscall *call;
        int delta_nr;

        void clear(void) {
            if(target_id == INVALID_ID) // already clear
                return;

            target_id = INVALID_ID;
            if(type == ADD_ENTRY || type == REMOVE_ENTRY)
                if(u.entry.name)
                    delete [] u.entry.name;
            if(type == REPLACE_ENTRY) {
                if(u.replace_entry.name)
                    delete [] u.replace_entry.name;
            }
            memset(&u, sizeof u, 0);
        }

        bool valid(void) {
            return target_id != INVALID_ID;
        }

        void add(int target_id, abst_file_type_t target_type, 
                 mode_t target_mode);
        void truncate(int target_id, off_t length);
        void pwrite(int target_id, const void* buf, 
                    size_t size, off_t off);
        void add_entry(int target_id, const char* name, int child_id);
        void remove_entry(int target_id, const char* name, 
                          int child_id);
        void replace_entry(int target_id, const char* name, 
                           int old_child_id, int new_child_id);
        void set_call(struct syscall* call, int delta_nr) {
            this->call = call;
            this->delta_nr = delta_nr;
        }

        void print(std::ostream& o) const;

        delta() {target_id = INVALID_ID;}
        ~delta() {clear();}
    };

    struct syscall {
        SyscallRec *enter, *exit;
		
        /* the object ids that this syscall modify. at our
           abstraction level, at most two objects can be
           modified by a syscall.  rmdir, unlink has only
           removes the directory entry, but doesn't
           actually remove things, since we don't model
           reference count.  similarly, link only affect
           one object.  rename affects one(rename within
           one dir) or two objects.

           when the change to an object is fsynced, set
           deltas[i] to INVALID_ID.
		   
           If there are two changes, deltas[0] should
           be done first, then deltas[1], i.e.
           deltas[1] depends on deltas[0] */
        struct delta deltas[2]; 
        syscall() {
            deltas[0].set_call(this, 0);
            deltas[1].set_call(this, 1);
        }
        syscall(SyscallRec *enter, SyscallRec *exit) {
            this->enter = enter;
            this->exit = exit;
            deltas[0].set_call(this, 0);
            deltas[1].set_call(this, 1);
        }
        void clear_deltas(void) {
            deltas[0].clear();
            deltas[1].clear();
        }
        void clear_delta(int n) {
            assert(n == 0 || n == 1);
            deltas[n].clear();
        }
    };
	
    typedef std::list<struct syscall*> syscall_list_t;
    typedef std::list<struct delta*> delta_list_t;
    typedef std::vector<struct delta*> delta_vec_t;

#define DECL_REPLAY(fn, Fn) \
    void replay_enter_##fn(Enter##Fn##Rec* enter, Exit##Fn##Rec* exit); \
    void replay_exit_##fn(Enter##Fn##Rec* enter, Exit##Fn##Rec* exit)

    void replay_syscall(SyscallRec *sr);
    DECL_REPLAY(creat, Open);
    DECL_REPLAY(ftruncate, Ftruncate);
    DECL_REPLAY(pwrite64, Pwrite64);
    DECL_REPLAY(mkdir, Mkdir);
    DECL_REPLAY(unlink, Unlink);
    DECL_REPLAY(rmdir, Rmdir);
    DECL_REPLAY(link, Link);
#if 0
    DECL_REPLAY(symlink, Symlink);
#endif
    DECL_REPLAY(rename, Rename);
    DECL_REPLAY(open, Open);
    // only their results need to be replayed
    DECL_REPLAY(close, Close);
    DECL_REPLAY(sync, Sync);
    DECL_REPLAY(fsync, Fsync);

#undef DECL_REPLAY

    meta *find_volatile_obj(const char* path_in_volatile);
    meta *make_obj_stable(const char* path_in_volatile,  bool create_p);
    bool make_delta_stable(AbstFS &fs, struct delta *delta);

    void find_obj_deltas(int id, delta_vec_t& deltas, delta_list_t&);

    struct shorter;
    struct longer;
    struct equal_byte;
    struct zeroed_byte;
    class diff_region;

    bool check_data(AbstFS& fs, AbstFS &recovered, 
                    delta_vec_t& data_deltas);

    void clear_syscalls(void);
    void internal_check(void);

    void on_sync_ops(void);
    void clear(void);

    SyscallPermuter* permuter(void);


    static const char* delta_names[DELTA_TYPE_MAX];

    // fd -> descriptor struct
    Sgi::hash_map<int, struct file_des> fds;

    // syscalls that are volatile (i.e. not entirely written to disk)
    syscall_list_t syscalls;

    // syscalls that have begun but haven't finished.
    // i.e. we haven't seen their *_res records yet.
    // actually, for the current implementation, there can be
    // only 1 ongoing syscall
    //syscall_list_t ongoing_syscalls;
    SyscallRec *ongoing_syscall;

    // now we store them in _obj_map too
    // objects that are written to disk, but their parents are not
    // obj_map_t stable_orphans;

    AbstFS _volatile_fs;

    int _last_op_is_sync;

    // signatures of valid FS that are supported by the
    // pending syscall list.  note, we only need to invalid
    // this when we apply a delta of a call from the pending
    // syscall list
    Sgi::hash_map<signature_t, bool> _valid_fs_sigs;

#if 0
    // shouldn't be combination.  should be powerset
    Combination _c;
#else
    Powerset _c;
#endif

    SyscallPermuter *_perm;

public:
    StableFS();
    virtual ~StableFS() {clear();}
    virtual void start(void);
    virtual void finish(void);
    virtual void replay(Record* rec);
    virtual int check(void);
    virtual void update();

#if 0
    virtual int mount(const char *target, 
                      unsigned long mountflags, const void *data);
#endif
    virtual void mount_path(const char* mnt);
    virtual void driver(FsTestDriverBase *driver);
};

std::ostream& operator<<(std::ostream& o, const StableFS::delta& d);
#endif

#endif
