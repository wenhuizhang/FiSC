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


#include "sysincl.h"
#include "mcl.h"
#include "crash.h"
#include "Fs.h"
#include "../crash/crash-options.h"
#include "storage-options.h"
#include "../driver/driver-options.h"

using namespace std;

Component *build_typical_fs_stack(const char* mnt)
{
    HostLib *ekm = sim()->new_ekm();

    Rdd *d = sim()->new_rdd();

    Fs *fs = new_fs(mnt, ekm); assert(fs);
    fs->plug_child(d);

    return fs;
}

void Fs::init(void)
{
    set_default_size();
    set_default_blk_size();
    set_default_fsck_mode();
    mount_sync_p = false;
    num_blks = size() / blk_size;
    check_config();
}

void Fs::mount(void) 
{
    int ret;
    ret = _ekm->systemf("sudo mount -t %s %s %s %s", type.c_str(),
                        mnt_opt.c_str(), children[0]->path(), path());
    if(ret != 0)
        if(get_option(ekm, disk_choice_ioctl))
            xe_abort("disk errors during mount");
        else
            error("mount", "mount failed!");
    if(!fat_p) {
        ret = _ekm->systemf("sudo chown -R %s %s", getenv("USER"), path());
        if(ret != 0)
            if(get_option(ekm, disk_choice_ioctl))
                xe_abort("disk errors during chown");
            else
                error("chown", "chown failed!");
    }
}

void Fs::umount(void) 
{
    int ret=-1, retires=get_option(fs, retries_on_failed_umount);
    // retry 3 times on umount
    ret = _ekm->systemf("sudo umount %s", path());
    while(ret != 0 && retires-- > 0) {
        sleep(1);
        ret = _ekm->systemf("sudo umount %s", path());
    }
}

void Fs::append_mnt_opt(string opt)
{
    if(mnt_opt.empty())
        mnt_opt += "-o " + opt;
    else
        mnt_opt += "," + opt;
}

void Fs::set_mnt_opts(void)
{
    append_mnt_opt("noatime");
    if(get_option(check, mount_sync)) {
        mount_sync_p = true;
        append_mnt_opt("sync");
    }
    if(get_option(check, mount_dir_sync))
        append_mnt_opt("dirsync");
}

void Fs::set_default_blk_size(void)
{
    int s = get_option(fs, blk_size);
    vector<int>::size_type i;
    for(i = 0; i < supported_blk_sizes.size(); i++)
        if(s == supported_blk_sizes[i]) {
            blk_size = s;
            return;
        }
	
    cout << type << "doesn't support block size " << s << "!" << endl;
    xi_assert(supported_blk_sizes.size() > 0, 
              "supported block sizes are not set! exit");
    blk_size = supported_blk_sizes[0];
}

void Fs::set_default_fsck_mode(void)
{
    enum fsck_mode m = (enum fsck_mode)get_option(fsck, mode);
    vector<int>::size_type i;
    for(i = 0; i < supported_fsck_modes.size(); i++)
        if(m == supported_fsck_modes[i]) {
            fsck_mode = m;
            return;
        }
    cout << type << "doesn't support fsck mode " << m << "!" << endl;
    xi_assert(supported_fsck_modes.size() > 0, 
              "supported fsck modes are not set! exit");
    fsck_mode = supported_fsck_modes[0];
}

void Fs::check_config(void)
{
    xi_assert(min_num_blks * blk_size <= size(),
              "%s too small.  make it at least %d bytes, %d sectors",
              children[0]->path(), min_num_blks * blk_size, 
              min_num_blks * blk_size/512);

    if(root_p && getuid() != 0 && geteuid() != 0)
        xi_panic("must be root to check %s", type.c_str());
}
 
Ext2::Ext2(const char* path, HostLib *ekm): Fs(path, ekm) 
{
    min_num_blks = 60;
    type = "ext2";
    fat_p = false;
    root_p = false;
    hole_p = true;
    supported_blk_sizes.push_back(1024);
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_force_repair);
    supported_fsck_modes.push_back(fsck_label_badblock);
}

void Ext2::init(void) 
{
    Fs::init();
    if(size())
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.ext2 -F -b %d %s %d", 
                      blk_size, children[0]->path(), 
                      size()/blk_size);
    else
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.ext2 -F -b %d %s", 
                      blk_size, children[0]->path());
}

void Ext2::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.ext2 -y %s", 
                      children[0]->path());
        break;
    case fsck_force_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.ext2 -y -f %s", 
                      children[0]->path());
        break;
    case fsck_label_badblock:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.ext2 -y -c %s", 
                      children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

#ifdef EEEE
Ext3::Ext3(const char* path, HostLib *ekm): Fs(path, ekm) 
{
    min_num_blks = 2048;
    type = "ext3";
    fat_p = false;
    root_p = false;
    hole_p = true;
    supported_blk_sizes.push_back(1024);
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_force_repair);
    supported_fsck_modes.push_back(fsck_label_badblock);
}

void Ext3::init(void) 
{
    Fs::init();
    if(size())
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.ext3 -F -j -b %d %s %d", 
                      blk_size, children[0]->path(), size()/blk_size);
    else
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.ext3 -F -j -b %d %s", 
                      blk_size, children[0]->path());
}

void Ext3::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.ext3 -y %s",
                      children[0]->path());
        break;
    case fsck_force_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.ext3 -y -f %s", 
                      children[0]->path());
        break;
    case fsck_label_badblock:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.ext3 -y -c %s", 
                      children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

void Ext3::set_mnt_opts(void)
{
    Fs::set_mnt_opts();
    if(get_option(check, journal_data))
        append_mnt_opt("data=journal");
    append_mnt_opt("commit=65535");
}

void Ext3::threads(threads_t &ths)
{
    ths.push_back(get_pid("kjournald"));
}
#else

// just to make it compile
Ext3::Ext3(const char* path, HostLib *ekm): Fs(path, ekm) 
{
    assert(0);
}

Ext3::Ext3(const char* path): Fs(path, true, -1) {
    hole_p = true; // support holes in files
    blk_size = 1024; // set block size 1024
}

void Ext3::init(void) {
    systemf("mkfs.ext3 -F -j -b %d %s", 
            blk_size, children[0]->path());
}

void Ext3::recover() {
    systemf("fsck.ext3 -y %s", children[0]->path());
}

void Ext3::mount(void) {
    int ret;
    ret = systemf("sudo mount -t ext3 -o commit=65535 %s %s",
                  children[0]->path(), path());
    // detect corrupted FS that cannot be mounted
    if(ret < 0) error("", "Can't mount target file system!");

    // may cut this
    ret = systemf("sudo chown -R %s %s", getenv("USER"), path());
    if(ret < 0) error("", "Can't chown target file system!");
}

void Ext3::umount(void) {
    systemf("sudo umount %s", path());
}

void Ext3::threads(threads_t &tids) {
    int pid;
    if((pid=get_pid("kjournald")) != -1)
        tids.push_back(pid);
    else
        xi_panic("can't get kjournald pid!");
}

#endif

ReiserFS::ReiserFS(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    min_num_blks = 8198 + 1 + 16 + 32;
    type = "reiserfs";
    fat_p = false;
    root_p = false;
    hole_p = false;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_rebuild_tree);
}

void ReiserFS::init(void) 
{
    Fs::init();
    // wierdly, reisefs size must be 64 * 1024 aligned. 
    num_blks = (size()/(64*1024))*(64*1024/blk_size);
    if(size())
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.reiserfs -f -f -b %d %s %d", 
                      blk_size, children[0]->path(), num_blks);
    else
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.reiserfs -f -f -b %d %s", 
                      blk_size, children[0]->path());
}

void ReiserFS::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.reiserfs --yes --fix-fixable  %s",
                      children[0]->path());
        break;
    case fsck_rebuild_tree:
        _ekm->systemf(FSPROG_ROOT 
                      "sbin/fsck.reiserfs --yes --rebuild-sb --rebuild-tree %s", 
                      children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

void ReiserFS::set_mnt_opts(void)
{
    Fs::set_mnt_opts();
    if(get_option(check, journal_data))
        append_mnt_opt("data=journal");
    append_mnt_opt("commit=65535");
}

void ReiserFS::threads(threads_t &ths)
{
    ths.push_back(get_pid("reiserfs/0"));
}

Reiser4::Reiser4(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    min_num_blks = 4096;
    type = "reiser4";
    fat_p = false;
    root_p = false;
    hole_p = true;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_rebuild_tree);
}

void Reiser4::init(void) 
{
    Fs::init();
    if(size())
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.reiser4 -f -y -b %d %s %d", 
                      blk_size, children[0]->path(), size()/blk_size);
    else
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.reiser4 -f -y -b %d %s", 
                      blk_size, children[0]->path());
}

void Reiser4::recover()
{
    int ret;
    switch(fsck_mode) {
    case fsck_repair:
        ret = _ekm->systemf(FSPROG_ROOT "sbin/fsck.reiser4 -y --fix %s",
                            children[0]->path());
        if(ret == 0)
            break;
        // --fix doesn't fix the problem.  fall thruough
    case fsck_rebuild_tree:
        ret = _ekm->systemf(FSPROG_ROOT 
                            "sbin/fsck.reiser4 -y --build-sb --build-fs %s", 
                            children[0]->path());
        if(ret)
            error("reiser4-cannot-repair", "reiser4 fsck can't repair");
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

void Reiser4::threads(threads_t &ths)
{
    ths.push_back(get_pid("ktxnmgrd:"));
    ths.push_back(get_pid("ent:"));
}

Jfs::Jfs(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    min_num_blks = 8192;
    type = "jfs";
    fat_p = false;
    root_p = false;
    hole_p = true;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_force_repair);
    supported_fsck_modes.push_back(fsck_replay_only);
}

void Jfs::init(void) 
{
    Fs::init();
    if(size())
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.jfs -q %s %d", 
                      children[0]->path(), size()/blk_size);
    else
        _ekm->systemf(FSPROG_ROOT "sbin/mkfs.jfs -q %s",
                      children[0]->path());
    // need this sync to make jfs utility 1.1.7 to work
    sync();
}

void Jfs::recover()
{

    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.jfs -p %s", children[0]->path());
        break;
    case fsck_force_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.jfs -p -f %s", 
                      children[0]->path());
        break;
    case fsck_replay_only:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.jfs -p --replay_journal_only %s",
                      children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
    // need this sync to make jfs utility 1.1.7 to work
    sync();
}

void Jfs::threads(threads_t &ths)
{
    ths.push_back(get_pid("jfsIO"));
    ths.push_back(get_pid("jfsCommit"));
    ths.push_back(get_pid("jfsSync"));
}

Xfs::Xfs(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    min_num_blks = 16 * 1024; // need two AGs for xfs_repair to work
    type = "xfs";
    fat_p = false;
    root_p = false;
    hole_p = true;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
}

void Xfs::init(void) 
{
    Fs::init();
    _ekm->systemf(FSPROG_ROOT "sbin/mkfs.xfs -f %s", children[0]->path());
}

void Xfs::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.xfs %s", children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

void Xfs::set_mnt_opts(void)
{
    // stop checking uuid, so we can copy the same disk content to
    // /dev/rdd0, /dev/rdd1, etc
    append_mnt_opt("nouuid");
    append_mnt_opt("noatime");

    // on xfs, -o sync,wsync == -o sync
    if(get_option(check, mount_sync)) {
        mount_sync_p = true;
        append_mnt_opt("sync");
        append_mnt_opt("wsync");
    }
    if(get_option(check, mount_dir_sync))
        append_mnt_opt("dirsync");
}

void Xfs::threads(threads_t &ths)
{
    ths.push_back(get_pid("xfslogd/0"));
    ths.push_back(get_pid("xfsdatad/0"));
    ths.push_back(get_pid("xfsbufd"));
    ths.push_back(get_pid("xfssyncd"));
}

msdos::msdos(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    //min_num_blks = 8192;
    min_num_blks = 8;
    type = "msdos";
    fat_p = true;
    root_p = true;
    hole_p = false;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
}

void msdos::init(void) 
{
    Fs::init();
    _ekm->systemf(FSPROG_ROOT "sbin/mkfs.msdos %s", children[0]->path());
}

void msdos::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.msdos -a %s", 
                      children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

vfat::vfat(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    //min_num_blks = 8192;	
    min_num_blks = 8;
    type = "vfat";
    fat_p = true;
    root_p = true;
    hole_p = false;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
}

void vfat::init(void) 
{
    Fs::init();
    _ekm->systemf(FSPROG_ROOT "sbin/mkfs.vfat %s", children[0]->path());
}

void vfat::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck.vfat -a %s", children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

ntfs::ntfs(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    //min_num_blks = 8192;
    min_num_blks = 257;
    type = "ntfs";
    fat_p = false;
    root_p = true;
    hole_p = true;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
}

void ntfs::init(void) 
{
    Fs::init();
    _ekm->systemf(FSPROG_ROOT "sbin/mkfs.ntfs %s", children[0]->path());
}

void ntfs::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "bin/ntfsfix %s", children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

hfs::hfs(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    min_num_blks = 257;
    type = "hfs";
    fat_p = false;
    root_p = true;
    hole_p = false;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_force_repair);
    supported_fsck_modes.push_back(fsck_replay_only);
}

void hfs::init(void) 
{
    Fs::init();
    // debian doesn't have a mkfs.hfs or mkfs.hfsplus.  has to use
    // our own complied one
    _ekm->systemf("sbin/newfs_hfs -h %s", children[0]->path());
}

// FIXME not sure if these commands are correct.  copied directly
// from fs-cmds.c
void hfs::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf( "sbin/fsck_hfs -q -f %s", 
                       children[0]->path());
        break;
    case fsck_force_repair:
        _ekm->systemf( "sbin/fsck_hfs -y -f %s", 
                       children[0]->path());
        break;
    case fsck_replay_only:
        _ekm->systemf( "sbin/fsck_hfs -p -f %s", 
                       children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

hfsplus::hfsplus(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    //min_num_blks = 8192;
    min_num_blks = 512;
    type = "hfsplus";
    fat_p = false;
    root_p = false;
    hole_p = false;
    supported_blk_sizes.push_back(4096);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_force_repair);
    supported_fsck_modes.push_back(fsck_replay_only);
}

void hfsplus::init(void) 
{
    Fs::init();
    // debian doesn't have a mkfs.hfsplus or mkfs.hfsplusplus.  has to use
    // our own complied one
    _ekm->systemf("sbin/newfs_hfs %s", children[0]->path());
}

// FIXME not sure if these commands are correct
void hfsplus::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf( "sbin/fsck_hfs -p -f %s", 
                       children[0]->path());
        break;
    case fsck_force_repair:
        _ekm->systemf( "sbin/fsck_hfs -y -f %s", 
                       children[0]->path());
        break;
    case fsck_replay_only:
        _ekm->systemf( "sbin/fsck_hfs -p -f %s", 
                       children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

ufs::ufs(const char* path, HostLib *ekm)
    : Fs(path, ekm) 
{
    //min_num_blks = 8192;
    min_num_blks = 32;
    type = "ufs";
    fat_p = false;
    root_p = false;
    // FIXME:  check if ufs supports holes
    hole_p = false;
    supported_blk_sizes.push_back(16*1024);
    supported_fsck_modes.push_back(fsck_repair);
    supported_fsck_modes.push_back(fsck_force_repair);
}

void ufs::init(void) 
{
    Fs::init();
    // debian doesn't have a mkfs.ufs or mkfs.ufsplus.  has to use
    // our own complied one
    _ekm->systemf(FSPROG_ROOT "sbin/newfs -O 2 %s", children[0]->path());
}

// FIXME not sure if these commands are correct
void ufs::recover()
{
    switch(fsck_mode) {
    case fsck_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck_ufs -y %s", 
                      children[0]->path());
        break;
    case fsck_force_repair:
        _ekm->systemf(FSPROG_ROOT "sbin/fsck_ufs -y -f %s", 
                      children[0]->path());
        break;
    default:
        xi_panic("unsupported fsck mode %d\n", fsck_mode);
    }
}

Fs *new_fs(const char* mnt, HostLib *ekm_handle)
{
    if(option_eq(fs, type, "ext2"))
        return new Ext2(mnt, ekm_handle);
    else if(option_eq(fs, type, "ext3"))
#ifdef EEEE
        return new Ext3(mnt, ekm_handle);
#else
    return new Ext3(mnt);
#endif
    else if(option_eq(fs, type, "JFS"))
        return new Jfs(mnt, ekm_handle);
    else if(option_eq(fs, type, "ReiserFS"))
        return new ReiserFS(mnt, ekm_handle);
    else if(option_eq(fs, type, "Reiser4"))
        return new Reiser4(mnt, ekm_handle);
    else if(option_eq(fs, type, "XFS"))
        return new Xfs(mnt, ekm_handle);
    else if(option_eq(fs, type, "vfat"))
        return new vfat(mnt, ekm_handle);
    else if(option_eq(fs, type, "msdos"))
        return new msdos(mnt, ekm_handle);
    else if(option_eq(fs, type, "ntfs"))
        return new ntfs(mnt, ekm_handle);
    else if(option_eq(fs, type, "hfs"))
        return new hfs(mnt, ekm_handle);
    else if(option_eq(fs, type, "hfsplus"))
        return new hfsplus(mnt, ekm_handle);
    else if(option_eq(fs, type, "ufs2"))
        return new ufs(mnt, ekm_handle);		
    else
        xi_panic("unknown file system %s", get_option(fs, type));
}
