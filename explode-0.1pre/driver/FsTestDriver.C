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

#include "marshal.h"
#include "writes.h"
#include "../storage/storage-options.h"


using namespace std;

#define CHECK_PATH(x) if(x<0) break

FsTestDriverBase::FsTestDriverBase(HostLib* ekm, Fs *fs)
{
    _ekm = ekm;
    _actual_fs = fs;

    _abst_fs.driver(this);

    if(get_option(check, o_sync))
        o_sync = O_SYNC;
    else
        o_sync = 0;

    // initialize test buffers -- we'll write these to the files
    // we create
    init_writes();

    // for file system that doesn't support holes, don't perform
    // writes at large offsets
    if(!_actual_fs->hole_p) 
        nwrites = 3;
}

void FsTestDriverBase::mount(void)
{
    _ekm->abstract_path(_actual_fs->path(), _abst_fs);

    mode_t m = umask(0);
    vvv_out << "old umask is " << m << "\n"
            << "new umask is " << umask(0) << endl;
}

void FsTestDriverBase::umount(void)
{
}

// fsync the given file
int FsTestDriverBase::do_fsync(const char *fn, bool fail) 
{
    vv_out << "OP: fsync(" << fn <<")"
           << endl;
    int fd = _ekm->open(fn, 0);
    if(fd < 0) {
        perror("open");
        error("open", "can't open %s", fn);
    }

    if(fail)
        sim()->pre_syscall();
    int ret = _ekm->fsync(fd);
    if(fail)
        sim()->post_syscall();

    if(!fail && ret < 0) {
        perror("fsync");
        if(option_ne(fs, type, "hfs")
           && option_ne(fs, type, "hfsplus"))
            error("fsync", "can't fsync %s", fn);
    }
    _ekm->close(fd);
    return ret;
}

// fsync every directory above last_file
void FsTestDriverBase::do_fsync_parents(const char *last_file)
{
    if(!get_option(check, fsync_parents))
        return;

    // find device of last_file
    struct stat last_file_stat;
    if(_ekm->stat(last_file, &last_file_stat) < 0) {
        perror("stat");
        error("fsync", "can't fsync");
    }
                
    // fsync every directory above last_file
    char *fn = xstrdup(last_file);
    while(char *slash = strrchr(fn, '/')) {
        // remove last component and bail if none left
        *slash = '\0';
        if (*fn == '\0')
            break;

        // bail out if we've changed filesystems
        struct stat fn_stat;
        if(_ekm->stat(fn, &fn_stat) < 0) {
            perror("stat");
            error("fsync", "can't stat %s for fsync", fn);
        }
        if (fn_stat.st_dev != last_file_stat.st_dev)
            break; 

        // fsync fn
        do_fsync(fn, false);
    }
    delete[] fn;
}

void FsTestDriverBase::mutate_creat(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_creat(path, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path, 0);
    xi_assert(ret >= 0, "test_write failed!");
    last_file = path;
}

void FsTestDriverBase::mutate_mkdir(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_mkdir(path, m);
    xi_assert(ret >= 0, "mkdir failed!");	
}

void FsTestDriverBase::mutate_unlink(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();

    _abst_fs.new_path(path, sizeof(path));
    ret = bi_creat(path, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path, 0);
    xi_assert(ret >= 0, "test_write failed!");

    // sync to make sure changes on disk
    bi_sync();

    ret = bi_unlink(path);
    xi_assert(ret >= 0, "unlink failed!");
}

void FsTestDriverBase::mutate_rmdir(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_mkdir(path, m);
    xi_assert(ret >= 0, "mkdir failed!");

    // sync to make sure changes on disk
    bi_sync();

    ret = bi_rmdir(path);
    xi_assert(ret >= 0, "rmdir failed!");
}

void FsTestDriverBase::mutate_link(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path_old, sizeof(path_old));
    ret = bi_creat(path_old, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path_old, 0);
    xi_assert(ret >= 0, "test_write failed!");

    // sync to make sure changes on disk
    bi_sync();
	
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_link(path_old, path);
    xi_assert(ret >= 0, "link failed!");
}

void FsTestDriverBase::mutate_rename_file_to_new(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path_old, sizeof(path_old));
    ret = bi_creat(path_old, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path_old, 0);
    xi_assert(ret >= 0, "test_write failed!");

    // sync to make sure changes on disk
    bi_sync();
	
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_rename(path_old, path);
    xi_assert(ret >= 0, "rename failed!");
}

void FsTestDriverBase::mutate_rename_file_to_exist(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path_old, sizeof(path_old));
    ret = bi_creat(path_old, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path_old, 0);
    xi_assert(ret >= 0, "test_write failed!");

    _abst_fs.new_path(path, sizeof(path));
    ret = bi_creat(path, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path, 0);

    // sync to make sure changes on disk
    bi_sync();
	
    ret = bi_rename(path, path);
    xi_assert(ret >= 0, "rename failed!");
}

void FsTestDriverBase::mutate_rename_dir_to_new(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path_old, sizeof(path_old));
    ret = bi_mkdir(path_old, m);
    // TODO add a file to this dir
    xi_assert(ret >= 0, "creat failed!");

    // sync to make sure changes on disk
    bi_sync();
	
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_rename(path_old, path);
    xi_assert(ret >= 0, "rename failed!");
}

void FsTestDriverBase::mutate_rename_dir_to_exist(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path_old, sizeof(path_old));
    ret = bi_mkdir(path_old, m);
    xi_assert(ret >= 0, "mkdir failed!");
    // creat a file in this dir
    bi_creat_file_in_dir(path_old);

    // FIXME: shouldn't creat a subdir under path_old
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_mkdir(path, m);
    xi_assert(ret >= 0, "creat failed!");
    // creat a file in this dir
    bi_creat_file_in_dir(path);

    bi_sync();
	
    ret = bi_rename(path_old, path);
    xi_assert(ret >= 0, "rename failed!");
}

void FsTestDriverBase::mutate_truncate_and_write(const char* &last_file)
{
}

void FsTestDriverBase::mutate_symlink(const char* &last_file)
{
}

void FsTestDriverBase::mutate_creat_exist(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_creat(path, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path, 0);
    xi_assert(ret >= 0, "test_write failed!");

    bi_sync();
	
    ret = bi_creat(path, m);
    if(ret >= 0)
        error("creat_exist", "creat should fail but didn't");
}

void FsTestDriverBase::mutate_mkdir_exist(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_mkdir(path, m);
    xi_assert(ret >= 0, "mkdir failed!");	

    bi_sync();
	
    ret = bi_mkdir(path, m);
    if(ret >= 0)
        error("mkdir_exist", "mkdir should fail but didn't");
}

void FsTestDriverBase::mutate_unlink_non_exist(const char* &last_file)
{
    int ret = -1;

    _abst_fs.new_path(path, sizeof(path));
    bi_unlink(path);
    if(ret >= 0)
        error("unlink_non_exist", "unlink should fail but didn't");
}

void FsTestDriverBase::mutate_rmdir_non_exist(const char* &last_file)
{
    int ret = -1;

    _abst_fs.new_path(path, sizeof(path));
    ret = bi_rmdir(path);
    if(ret >= 0)
        error("rmdir_non_exist", "rmdir should fail but didn't");
}

void FsTestDriverBase::mutate_link_non_exist(const char* &last_file)
{
    int ret = -1;

    _abst_fs.new_path(path_old, sizeof(path_old));
    _abst_fs.new_path(path, sizeof(path));
    bi_link(path_old, path);
    if(ret >= 0)
        error("link_non_exist", "link should fail but didn't");
}

void FsTestDriverBase::mutate_link_exist(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path_old, sizeof(path_old));
    ret = bi_creat(path_old, m);
    xi_assert(ret >= 0, "creat failed!");
    ret = bi_test_write(path_old, 0);
    xi_assert(ret >= 0, "test_write failed!");
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_creat(path, m);
    xi_assert(ret >= 0, "creat failed!");
	
    // sync to make sure changes on disk
    bi_sync();

    bi_link(path_old, path);
    if(ret >= 0)
        error("link_exist", "link should fail but didn't");
}

void FsTestDriverBase::mutate_rename_non_exist(const char* &last_file)
{
    int ret = -1;
    _abst_fs.new_path(path_old, sizeof(path_old));
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_rename(path_old, path);
    if(ret >= 0)
        error("rename_non_exist", "rename should fail but didn't");
}

void FsTestDriverBase::mutate_rmdir_non_empty(const char* &last_file)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    _abst_fs.new_path(path, sizeof(path));
    ret = bi_mkdir(path, m);
    xi_assert(ret >= 0, "mkdir failed!");
    // creat a file in this dir
    bi_creat_file_in_dir(path);
	
    // sync to make sure changes on disk
    bi_sync();

    ret = bi_rmdir(path);
    if(ret >= 0)
        error("rmdir_non_empty", "removed non-empty dir %s!", path);
}

void FsTestDriverBase::mutate_all_op(const char* &last_file)
{
    int n = get_option(fstest, num_ops);
    int op;
    int ret = -1;
    mode_t m = _actual_fs->get_mode();

    op = xe_random(n);
    switch (op) {
    case 0: // creat
        _abst_fs.new_path(path, sizeof(path));
        ret = bi_creat(path, m);
        if(ret >= 0) {
            bi_test_write(path, 0);
            last_file = path;
        }
        break;
    case 1: // mkdir
        _abst_fs.new_path(path, sizeof(path));
        bi_mkdir(path, m);
        break;
    case 2: // unlink
        ret = _abst_fs.choose_file(path, sizeof(path));
        CHECK_PATH(ret);
        bi_unlink(path);
        break;
    case 3: // rmdir
        ret = _abst_fs.choose_empty_dir(path, sizeof(path));
        CHECK_PATH(ret);
        bi_rmdir(path);
        break;
    case 4: //link
        ret = _abst_fs.choose_file(path_old, sizeof(path_old));
        CHECK_PATH(ret);
        _abst_fs.new_path(path, sizeof(path));
        bi_link(path_old, path);
        break;
    case 5: // rename
        ret = _abst_fs.choose_nonroot(path_old, sizeof(path_old));
        CHECK_PATH(ret);
        //TODO: path shouldn't be a children of path_old
        _abst_fs.new_path(path, sizeof(path));
        bi_rename(path_old, path);
        break;
    case 6: // truncate & write
        ret = _abst_fs.choose_reg_file(path, sizeof path);
        CHECK_PATH(ret);
        bi_test_truncate(path);
        last_file = path;		
        break;
    case 7: // symlink
        ret = _abst_fs.choose_any(path_old, sizeof(path_old));
        CHECK_PATH(ret);
        _abst_fs.new_path(path, sizeof(path));
        bi_symlink(path_old, path);
        break;
    default:
        break;
    }
}

void FsTestDriverBase::mutate(void) 
{
    v_out <<"\nBefore FS operation:\n" << _abst_fs;

    _last_file = NULL;

    if(option_eq(fstest, mutate, "all_op"))
        mutate_all_op(_last_file);
    else if(option_eq(fstest, mutate, "creat"))
        mutate_creat(_last_file);
    else if(option_eq(fstest, mutate, "mkdir"))
        mutate_mkdir(_last_file);
    else if(option_eq(fstest, mutate, "unlink"))
        mutate_unlink(_last_file);
    else if(option_eq(fstest, mutate, "rmdir"))
        mutate_rmdir(_last_file);
    else if(option_eq(fstest, mutate, "link"))
        mutate_link(_last_file);
    // TODO: should check atomic rename
    else if(option_eq(fstest, mutate, "rename_file_to_new"))
        mutate_rename_file_to_new(_last_file);
    else if(option_eq(fstest, mutate, "rename_file_to_exist"))
        mutate_rename_file_to_exist(_last_file);
    else if(option_eq(fstest, mutate, "rename_dir_to_new"))
        mutate_rename_dir_to_new(_last_file);
    else if(option_eq(fstest, mutate, "rename_dir_to_exist"))
        mutate_rename_dir_to_exist(_last_file);
    else if(option_eq(fstest, mutate, "truncate_and_write"))
        mutate_truncate_and_write(_last_file);
    else if(option_eq(fstest, mutate, "symlink"))
        mutate_symlink(_last_file);
    // the ones that should fail
    else if(option_eq(fstest, mutate, "creat_exist"))
        mutate_creat_exist(_last_file);
    else if(option_eq(fstest, mutate, "mkdir_exist"))
        mutate_mkdir_exist(_last_file);
    else if(option_eq(fstest, mutate, "unlink_non_exist"))
        mutate_unlink_non_exist(_last_file);
    else if(option_eq(fstest, mutate, "rmdir_non_exist"))
        mutate_rmdir_non_exist(_last_file);
    else if(option_eq(fstest, mutate, "link_non_exist"))
        mutate_link_non_exist(_last_file);
    else if(option_eq(fstest, mutate, "link_exist"))
        mutate_link_exist(_last_file);
    else if(option_eq(fstest, mutate, "rename_non_exist"))
        mutate_rename_non_exist(_last_file);
    else if(option_eq(fstest, mutate, "rmdir_not_empty"))
        mutate_rmdir_non_empty(_last_file);
    else
        xi_panic("unrecognized fstest::mutate %s",
                 get_option(fstest, mutate));

    v_out << "\nAfter FS operation:\n" << _abst_fs;

    // check abst_fs == actual_fs
    AbstFS a_fs;
    a_fs.driver(this);
    _ekm->abstract_path(_actual_fs->path(), a_fs);
    if(a_fs.compare(_abst_fs)) {
        cout << "Abstract FS:\n";
        _abst_fs.print(cout);
        cout << "Actual FS:\n";
        a_fs.print(cout);
        error("inconsistent", "abstract fs doesn't match actual fs!");
    }
}

signature_t FsTestDriverBase::get_sig(void)
{
    // XXX. add stable fs, or log
    //return _abst_fs.get_sig_no_name();
    // XXX why with name????
    return _abst_fs.get_sig_with_name();
    //return _abst_fs.get_sig_no_name();
}

// this macro prettify the messy code a little bit.  only a little
// bit...
#define BISIM(need_close, fn, args...) \
sim()->pre_syscall(); \
int ret = _ekm->fn(args); \
sim()->post_syscall(); \
if (ret < 0) { \
	perror("can't "#fn" on actual file system"); \
} else { \
	int ret2; \
	if (need_close) { \
		ret2 = _ekm->close(ret);\
		xi_assert(ret2 >= 0, "can't close file"); \
	} \
	ret2 = _abst_fs.fn(args); \
	xi_assert(ret2 >= 0, "can "#fn" on actual FS but not abstract FS"); \
} \
return ret

int FsTestDriverBase::bi_unlink(const char *pathname) 
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<pathname<<")\n"; 
    BISIM(0, unlink, pathname);
}

int FsTestDriverBase::bi_rmdir(const char *pathname) 
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<pathname<<")\n"; 
    BISIM(0, rmdir, pathname);
}

int FsTestDriverBase::bi_creat(const char *pathname, mode_t mode)
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<pathname<<","<<mode<<")\n"; 
    int ret;

    if(o_sync == O_SYNC) {
        sim()->pre_syscall();
        ret = _ekm->creat_osync(pathname, mode);
        sim()->post_syscall();
    } else {
        sim()->pre_syscall();
        ret = _ekm->creat(pathname, mode);
        sim()->post_syscall();
    }

    if (ret < 0) {
        perror("can't creat on actual file system");
    } else {
        int ret2;
        ret2 = _ekm->close(ret);
        xi_assert(ret2 >= 0, "can't close file");

        ret2 = _abst_fs.creat(pathname, mode);
        xi_assert(ret2>=0, "can creat on actual FS but not abstract FS");
#if 0
        if(o_sync == O_SYNC) {
            // need to fsync inode
            do_fsync(path);
            //after_fsync(path, FS_FSYNC);
        }
#endif
    }

    return ret;
}

int FsTestDriverBase::bi_mkdir(const char *pathname, mode_t mode)
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<pathname<<","<<mode<<")\n"; 
    BISIM(0, mkdir, pathname, mode);
}

int FsTestDriverBase::bi_link(const char *oldpath, const char* newpath)
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<oldpath<<","<<newpath<<")\n"; 
    BISIM(0, link, oldpath, newpath);
}

int FsTestDriverBase::bi_symlink(const char *oldpath, const char* newpath)
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<oldpath<<","<<newpath<<")\n"; 
    BISIM(0, symlink, oldpath, newpath);
}

int FsTestDriverBase::bi_rename(const char *oldpath, const char* newpath)
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<oldpath<<","<<newpath<<")\n"; 
    BISIM(0, rename, oldpath, newpath);
}

#include <unistd.h>
int FsTestDriverBase::bi_truncate(const char* pathname, off_t length)
{
    vv_out<<"OP: "<<__FUNCTION__<<"("<<pathname<<","<<length<<")\n"; 
    int fd = _ekm->open(pathname, O_WRONLY | o_sync);
    if (fd < 0) {
        perror("open");
        abort();
        return -1;
    }

    //sim()->pre_syscall();
    int ret = _ekm->ftruncate(fd, length);
    //sim()->post_syscall();
    if (ret < 0)
        perror("can't ftruncate on actual file system");
    else {
        int ret2 = _abst_fs.truncate(pathname, length);
        xi_assert(ret2 >= 0, "can truncate on actual FS but not abstract FS");
    }
    _ekm->close(fd);
    return ret;
}

void FsTestDriverBase::bi_sync(void)
{
    sim()->pre_syscall();
    _ekm->sync();
    after_sync();
    sim()->post_syscall();
}

// TODO: should just test tuncate...
int FsTestDriverBase::bi_test_truncate(const char* pathname)
{
    struct stat st;
    _abst_fs.stat(pathname, &st);

    int i = next_write_by_size(st.st_size);
    if (i > nwrites) assert(0);

    // "randomly" choose another size
    unsigned inc = xe_random(nwrites-1);
    int j = (i + inc) % nwrites;
#if 0
    if(OUTPUT_EXTRA_VERBOSE)
        cout << "truncate to " << file_sizes[j] << endl;
#endif
    int ret = bi_truncate(pathname, writes[j][0].size);

    if(ret < 0)
        return ret;
    // update sig of abstract file in case we truncate to smaller size
    if (i > j) 
        ;
#if 0
    _abst_fs.pwrite(pathname, writes[j][0].buf, 
                    writes[j][0].len, writes[j][0].off);
#endif
    else
        // if we increase the size, write
        while(i <= j)
            ret = bi_test_write(pathname, i++);

    return ret;
}

int FsTestDriverBase::bi_test_write(const char *pathname, unsigned i)
{
    if(!_actual_fs->hole_p)
        nwrites = 3;
    int fd = _ekm->open(pathname, O_WRONLY | o_sync);
    if(fd < 0)
        return fd;
    xi_assert(fd>=0, "can't open %s", pathname);
    //sim()->pre_syscall();
    int ret = _ekm->pwrite(fd, writes[i][0].buf, 
                           writes[i][0].len, writes[i][0].off);
    //sim()->post_syscall();
    perror(NULL);
    xi_assert(ret>= 0, "can't pwrite %s", pathname);
    _ekm->close(fd);
    // update sig of abstract file
    _abst_fs.pwrite(pathname, writes[i][0].buf, 
                    writes[i][0].len, writes[i][0].off);
    vv_out<<"OP: "<<__FUNCTION__<<"("<<pathname<<","<<i<<")\n"; 
    return ret;
}

void FsTestDriverBase::bi_creat_file_in_dir(const char* path)
{
    int ret = -1;
    mode_t m = _actual_fs->get_mode();
    char path_new[MAX_PATH];

    strcpy(path_new, path);
    strcat(path_new, "/");
    int dir_len = strlen(path_new);
    _abst_fs.new_name(path_new + dir_len, sizeof path_new - dir_len);
    ret = bi_creat(path_new, m);
    xi_assert(ret >= 0, "creat in dir %s failed!", path);
    // TODO: should write some data
}


void FsSyncTestDriver::umount(void) 
{
    char *data;
    int len;

    if(!get_option(check, umount)) {
        // need to stop trace, even if we're not checking crashes
        sim()->stop_trace();
        return;
    }

    // use the advanced interface to check, since we don't want to
    // call choose here.
    permuter()->check_now_crashes_ex(this, &FsSyncTestDriver::check_sync);
}


void FsSyncTestDriver::mutate(void)
{
    FsTestDriverBase::mutate();

    if(_last_file) {
        if(get_option(check, o_sync)) {
            do_fsync_parents(_last_file);
            after_fsync(_last_file);
        }
		
        if(get_option(check, fsync) && (xe_random(2) == 1)) {
            int ret;
            ret = do_fsync(_last_file, true);
            if(ret >= 0) {
                do_fsync_parents(_last_file);
                after_fsync(_last_file);
            }
        }
    }
	
    if(get_option(check, mount_sync)) {
        after_sync();
        return;
    }

    if(get_option(check, sync) && xe_random(2) == 1)
        bi_sync();
}

void FsSyncTestDriver::after_sync()
{
    check_now_crashes(&FsSyncTestDriver::check_sync);
}

void FsSyncTestDriver::after_fsync(const char* fn)
{
    int ret;

    AbstFS::meta *o;
    ret = _abst_fs.stat(fn, o);
    assert(ret >= 0 && o && _abst_fs.is_file(o));

    check_now_crashes(&FsSyncTestDriver::check_fsync, fn, o);
}

int FsSyncTestDriver::check_sync(void)
{
    AbstFS recovered_fs(this);
    _ekm->abstract_path(_actual_fs->path(), recovered_fs);

    if(_abst_fs.compare(recovered_fs)) {
        cout<<"RecoveredFS:\n";
        recovered_fs.print(cout);
        cout<<"SyncStableFS:\n";
        _abst_fs.print(cout);
        // won't return
        error("sync or mount sync or umount", "recovered fs != stable fs");
    }
    return CHECK_SUCCEEDED;
}

int FsSyncTestDriver::check_fsync(const char* fn, AbstFS::meta *fo)
{
    AbstFS::file *file = CAST(AbstFS::file*, fo);
    AbstFS::meta *o = NULL;
    AbstFS recovered_fs(this);
    _ekm->abstract_path(_actual_fs->path(), recovered_fs);

    if(recovered_fs.stat(fn, o) < 0) {
        cout<<"RecoveredFS:\n";
        recovered_fs.print(cout);
        cout<<"StableFile: " << fn << "\n";
        file->print(cout);
		
        if(!get_option(check, ignore_missing))
            error("fsync or osync",
                  "recovered file is missing");
        else
            cout<<"recovered file is missing\n";
    } else if(recovered_fs.compare_obj(o, file)) {
        cout<<"RecoveredFS:\n";
        recovered_fs.print(cout);
        cout<<"RecoveredFile: ";
        recovered_fs.print_obj(o, NULL, cout);
        cout<<"StableFile: " << fn<< "\n";
        file->print(cout);
        error("fsync or osync",
              "recovered file != stable file");
    } else
        vv_out <<"StableFile == recovered file\n";
    return CHECK_SUCCEEDED;
}

#ifdef CONF_EKM_LOG_SYSCALL
ReasonableFsTestDriver::ReasonableFsTestDriver(HostLib *ekm, Fs *fs)
    : FsTestDriverBase(ekm, fs)
{
    _stable_fs = new StableFS;
    _stable_fs->driver(this);
    _stable_fs->mount_path(_actual_fs->path());
}

ReasonableFsTestDriver::~ReasonableFsTestDriver()
{
    if(_stable_fs)
        delete _stable_fs;
}

void ReasonableFsTestDriver::mutate()
{
    FsTestDriverBase::mutate();

    // FIXME: this is a total hack. force a sync every 8 deltas
    if(_pending_deltas > MAX_SET-2)
        bi_sync();
}


void ReasonableFsTestDriver::mount() 
{
    FsTestDriverBase::mount();
    _pending_deltas = 0;
}


void ReasonableFsTestDriver::umount()
{
    permuter()->check_all_crashes_ex(_stable_fs, &StableFS::check);
}

int ReasonableFsTestDriver::bi_unlink(const char *pathname)
{
    int ret = FsTestDriverBase::bi_unlink(pathname);
    if(ret >= 0)
        _pending_deltas += 1;
    return ret;
}

int ReasonableFsTestDriver::bi_rmdir(const char *pathname)
{
    int ret = FsTestDriverBase::bi_rmdir(pathname);
    if(ret >= 0)
        _pending_deltas += 1;
    return ret;
}

int ReasonableFsTestDriver::bi_creat(const char *pathname, mode_t mode)
{
    int ret = FsTestDriverBase::bi_creat(pathname, mode);
    if(ret >= 0)
        _pending_deltas += 2;
    return ret;
}

int ReasonableFsTestDriver::bi_mkdir(const char *pathname, mode_t mode)
{
    int ret = FsTestDriverBase::bi_mkdir(pathname, mode);
    if(ret >= 0)
        _pending_deltas += 2;
    return ret;
}

int ReasonableFsTestDriver::bi_link(const char *oldpath, const char* newpath)
{
    int ret = FsTestDriverBase::bi_link(oldpath, newpath);
    if(ret >= 0)
        _pending_deltas += 1;
    return ret;
}

int ReasonableFsTestDriver::bi_symlink(const char *oldpath, const char* newpath)
{
    int ret = FsTestDriverBase::bi_symlink(oldpath, newpath);
    if(ret >= 0)
        _pending_deltas += 1;
    return ret;
}

int ReasonableFsTestDriver::bi_rename(const char *oldpath, const char* newpath)
{
    int ret = FsTestDriverBase::bi_rename(oldpath, newpath);
    if(ret >= 0)
        _pending_deltas += 2;    
    return ret;
}

void ReasonableFsTestDriver::bi_sync(void)
{
    FsTestDriverBase::bi_sync();
    _pending_deltas = 0;
}

#endif

FsTestDriverBase* new_fs_driver(HostLib *ekm, Fs *fs)
{
    FsTestDriverBase *driver;
    if(get_option(stablefs, sync))
        driver = new FsSyncTestDriver(ekm, fs);
#ifdef CONF_EKM_LOG_SYSCALL
    else
        driver = new ReasonableFsTestDriver(ekm, fs);
#endif
    xi_assert(driver, "Cannot create FS test driver!");
    return driver;
}

