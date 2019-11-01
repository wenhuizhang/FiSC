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


#ifndef __FSTESTDRIVER_H
#define __FSTESTDRIVER_H

#include "TestDriver.h"
#include "storage/Fs.h"
#include "AbstFS.h"
#include "StableFS.h"

class FsTestDriverBase:public TestDriver
{
    friend class AbstFS;
    friend class StableFS;
protected:
    HostLib *_ekm;
    AbstFS _abst_fs;
    Fs *_actual_fs;
    int o_sync;
    char path[MAX_PATH], path_old[MAX_PATH];

    const char* _last_file;

    virtual void bi_creat_file_in_dir(const char* path);

    virtual int bi_unlink(const char *pathname);
    virtual int bi_rmdir(const char *pathname);
    virtual int bi_creat(const char *pathname, mode_t mode);
    virtual int bi_mkdir(const char *pathname, mode_t mode);
    virtual int bi_link(const char *oldpath, const char* newpath);
    virtual int bi_symlink(const char *oldpath, const char* newpath);
    virtual int bi_rename(const char *oldpath, const char* newpath);
    virtual int bi_truncate(const char* pathname, off_t length);
    virtual void bi_sync(void);

    virtual int bi_test_truncate(const char* pathname);
    virtual int bi_test_write(const char *pathname, unsigned i);



    int do_fsync(const char *fn, bool fail);
    void do_fsync_parents(const char *fn);

    /* called after every sync or umount */
    virtual void after_sync(void) {}
    /* called after every fsync or operation to an O_SYNC opened file */
    virtual void after_fsync(const char* filename) {}

    void mutate_all_op(const char *&last_file);
    void mutate_creat(const char *&last_file);
    void mutate_mkdir(const char *&last_file);
    void mutate_unlink(const char *&last_file);
    void mutate_rmdir(const char *&last_file);
    void mutate_link(const char *&last_file);
    void mutate_rename_file_to_new(const char *&last_file);
    void mutate_rename_file_to_exist(const char *&last_file);
    void mutate_rename_dir_to_new(const char *&last_file);
    void mutate_rename_dir_to_exist(const char *&last_file);
    void mutate_truncate_and_write(const char *&last_file);
    void mutate_symlink(const char *&last_file);

    void mutate_creat_exist(const char *&last_file);
    void mutate_mkdir_exist(const char *&last_file);
    void mutate_unlink_non_exist(const char *&last_file);
    void mutate_rmdir_non_exist(const char *&last_file);
    void mutate_rmdir_non_empty(const char *&last_file);
    void mutate_link_exist(const char *&last_file);
    void mutate_link_non_exist(const char *&last_file);
    void mutate_rename_non_exist(const char *&last_file);

public:
    FsTestDriverBase(HostLib* ekm, Fs *fs);
    virtual ~FsTestDriverBase() {}

    virtual void mount(void);
    virtual void umount(void);
    virtual void mutate(void);
    virtual signature_t get_sig(void);

    //StableFS *stable_fs(void) {return _stable_fs;}
    Fs *actual_fs(void) {return _actual_fs;}
};

class FsSyncTestDriver: public FsTestDriverBase
{
public:
    FsSyncTestDriver(HostLib *ekm, Fs *fs): FsTestDriverBase(ekm, fs)
    { _new_interface_p = true; }

protected:
    int check_sync();
    int check_fsync(const char* fn, AbstFS::meta *o);

    virtual void umount(void);
    virtual void mutate(void);

    virtual void after_sync(void);
    virtual void after_fsync(const char* fn);
};

#ifdef CONF_EKM_LOG_SYSCALL
class ReasonableFsTestDriver: public FsTestDriverBase
{
    friend class StableFS;
public:
    ReasonableFsTestDriver(HostLib *ekm, Fs *fs);
    virtual ~ReasonableFsTestDriver();

    virtual Replayer *get_replayer(void) {return _stable_fs;}

    virtual void mount(void);
    virtual void umount(void);
    virtual void mutate(void);

protected:
    StableFS *_stable_fs;

    int _pending_deltas;  // count the number of deltas are pending

    // overload these functions to count the # of pending deltas
    virtual int bi_unlink(const char *pathname);
    virtual int bi_rmdir(const char *pathname);
    virtual int bi_creat(const char *pathname, mode_t mode);
    virtual int bi_mkdir(const char *pathname, mode_t mode);
    virtual int bi_link(const char *oldpath, const char* newpath);
    virtual int bi_symlink(const char *oldpath, const char* newpath);
    virtual int bi_rename(const char *oldpath, const char* newpath);
    virtual void bi_sync(void);
};
#endif

class  LogFsTestDriver: public FsTestDriverBase
{
};

FsTestDriverBase* new_fs_driver(HostLib *ekm, Fs *fs);

#endif
