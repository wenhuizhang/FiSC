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


/* AbstFS is just at DAG.  Non-leaf nodes are directories.  Leaf
   nodes are files.  Each edge corresponds to a directory entry.

   Also needs to record opened file descriptors. This is hard!!!
   So I decided not to do it.  IMPORTANT: we don't work on file
   descriptor what-so-ever. only absolute file names. We do this
   because we don't want to keep any internal state (sorta like
   NFS), and we want to avoid the complexity to model complicated
   things such as open flags, permissions, ACL, lock/unlock....

   A symbolic link is a special file which stores a path to
   another node

   We need a few extra functionalities from this abstract file
   system: 

   1. return a list of nodes that satisfy a criteria, so we can do
   explode_random on this list

   2. given a node, we can easily walk from this node up to the
   file system root.

   TODO: [not to do any more]

   1. differet flags/system calls
   O_NONBLOCK or O_NDELAY
   O_DIRECT
   O_ASYNC

   2. multi-thread?

   driver approach mixes checking and testing.  bad!

   can cross check OS + libc.  libc provides POSIX interface.
   underlying hardware provides same interface.  run it on VM.
   need to have model checking VM
   
   above libc layer, can check applications.  this require we have
   a model checking os + libc.


   No opened file descriptor table, current working directory.
   this should just be a simplified user-land file system.  when
   it needs to know the kernel state, just do a system call.
   Don't want to Keep one thing at two different places and make
   the two copies sync

   why do we need an abstract file system implementation?  use
   another file system.  we only need an abstraction function to
   abstract both file system into an abstract file system model.
   We don't need to implement all the file system operations!!!

   implementing an abstract file system with some level of details
   is non-trivial!  think of the permission checks. ACL. name
   resolution.  different open flags.  should just cross check a
   file system with a different one.  or even cross check it
   against itself without failing functions and crashes.

   TODO:
   rewrite the signature part
*/

#ifndef __ABSTFS_H
#define __ABSTFS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//#define DEBUG_ABSTFS

#include <map>
#include "hash_map_compat.h"
#include <vector>
#include <functional>
#include <iostream>
#include <list>

#include "mcl.h"
#include "Archive.h"
#include "Component.h"
#include "data.h"

//#define CAST(T, x) static_cast<T>(x)
#define INVALID_ID (-1)

/* constants */
#define ALL_PERM (00777)

// max depth of recursive when visiting a file system.
#define MAX_DEPTH (MAX_PATH/2)

class FsTestDriverBase;
class AbstFS
{
public:

    // only supports regular files, directories, and symbolic links
    typedef enum abst_file_type_e {
        ABST_FILE=0, ABST_DIR=1, ABST_SYM_LINK=2, 
        ABST_MAX} abst_file_type_t;
    static const char type_char[ABST_MAX]; // for printing

    typedef enum visit_status_e {
        VISIT_CONT=0, VISIT_STOP=1,
    } visit_status_t;
	
    class InvalidFS {
    public:
        std::string cause;
        InvalidFS() {}
        InvalidFS(const char* s): cause(s) {}
    };

    // file name -> id map.  This implements the directory
    // entry table.  use map rather than hash_map so the names
    // are always sorted
    class meta;
    typedef std::map<const char*, int, ltstr> entry_map_t;

    typedef std::map<int, int> id_map_t;
	
    // name data
    typedef struct namei_s {
        int want_parent:1;
        //int follow_link:1; // ain't we always follow the symlinks?
        meta* obj;
        char name[MAX_NAME]; // holds the file name
    } namei_t ;
    static namei_t namei_init;
    static namei_t parent_namei_init;

    // predicate on meta objects
    typedef bool (AbstFS::*obj_pred_fn_t) (meta *);
	
    // metadata generic to everything.  one should never call
    // new meta().
    class meta {
    public:
        // unique id for this object
        int id; // can apply symmetric reducation
        int ref; // reference count
        abst_file_type_t type;

        /* 
           S_IFSOCK   0140000   socket
           S_IFLNK    0120000   symbolic link
           S_IFREG    0100000   regular file
           S_IFBLK    0060000   block device
           S_IFDIR    0040000   directory
           S_IFCHR    0020000   character device
           S_IFIFO    0010000   fifo */
        mode_t mode;
        // more generic metadata go here. e.g. uid/gid, acl
    public:
        meta(abst_file_type_t t, mode_t m): 
            id(INVALID_ID), ref(0), type(t), mode(m) {}

        virtual ~meta() {}
        virtual void copy(meta *o) = 0;
        virtual meta* clone(void) = 0;
        void copy_meta(meta *o) {
            id = o->id;
            ref = o->ref;
            //type = o->type;
            mode = o->mode;
        }
    };

    // file data
    // Should just use incremental hash for file data?  when write a
    // block, compute hash for this block, then compute new hash.  On
    // the other hand, the pickling process should be able to
    // understand holes.
    class file: public meta {
    public:
        off_t size;
        signature_t sig; // is kept up-to-date with data d
        data d;
    public:
        file(): meta(ABST_FILE, ALL_PERM), size(0), sig(0) {}
        file(mode_t m): meta(ABST_FILE, m), 
			size(0), sig(0) {}
        file(mode_t m, off_t s, signature_t sg): meta(ABST_FILE, m),
                                                 size(s), sig(sg) {}
        virtual void print(std::ostream &o);
        virtual void copy(meta *m){
            file *o = CAST(file*, m);
            assert(type == o->type);
            copy_meta(o);
            size = o->size;
            sig = o->sig;
            d.copy(o->d);
        }
        virtual meta* clone(void) {
            file* o = new file;
            o->copy(this);
            return o;
        }

        size_t pwrite(const void *buf, size_t size, 
                      off_t off);
        size_t pread(void *buf, size_t size, off_t off);
        int truncate(off_t length);
    };
	
    // directory data
    class dir: public meta {
    public:
        entry_map_t entries;
        // dir can only have 1 parent. no hard link is
        // allowed to directories!
        dir *paren;
    public:
        dir(): meta(ABST_DIR, ALL_PERM), paren(NULL) {}
        dir(mode_t m): meta(ABST_DIR, m), paren(NULL) {}
        virtual ~dir() {clear();}
        virtual void copy(meta *m) {
            clear();
            assert(type == m->type);
            dir *o = CAST(dir*, m);
            copy_meta(o);
            entry_map_t::iterator it;
            for(it=o->entries.begin();it!=o->entries.end();it++)
			
		add_entry((*it).first, (*it).second);
            paren = o->paren;
        }
        virtual meta* clone(void) {
            dir* o = new dir;
            o->copy(this);
            return o;
        }
        void clear(void) {
            entry_map_t::iterator it;
            for(it = entries.begin(); it != entries.end(); it ++)
                delete [] (*it).first;
            entries.clear();
        }

        void add_entry(const char* name, int id);
        int rm_entry(const char* name);
        int get_entry(const char* name);
        void set_paren(dir *p) {paren = p;}
        dir* get_paren(void) {return paren;}
    };

    // symbolic link
    class sym_link: public meta {
    public:
        char path[MAX_PATH]; // path to another file system object
    public:
        sym_link(): meta(ABST_SYM_LINK, ALL_PERM) 
        {memset(path, 0, sizeof(path));}
        sym_link(const char* link): 
            meta(ABST_SYM_LINK, ALL_PERM) {
            xi_assert(strlen(link) < sizeof (path),
                           "symlink longer than maximum path");
            strcpy(path, link);
        }
        virtual void copy(meta* m) {
            sym_link *o = CAST(sym_link*, m);
            assert(type == o->type);
            copy_meta(o);
            memcpy(path, o->path, sizeof path);
        }
        virtual meta* clone(void) {
            sym_link* o = new sym_link;
            o->copy(this);
            return o;
        }
    };

    // visitors
    class visitor {
    public:
        bool want_dangling;
        visitor() {want_dangling = false;}
        virtual ~visitor() {}
        virtual visit_status_t 
        pre_visit(meta* parent, const char* name, 
                  meta* obj, int depth)
        {return VISIT_CONT;}

        virtual visit_status_t 
        post_visit(meta* parent, const char* name, 
                   meta* obj, int depth)
        {return VISIT_CONT;}
    };

    class printer: public visitor {
    private:
        std::ostream *o;
    public:
        printer(std::ostream &o) {this->o = &o; want_dangling = true;}
        visit_status_t pre_visit(meta* parent, const char* name, 
                                 meta* obj, int depth);
    };
    class visit_order: public visitor {
    private:
        id_map_t *order;
        bool pre;
        int cur;
    public:
        visit_order(id_map_t* o, bool p) {
            order = o; pre = p; cur = 0;
        }
        void add_to_order(meta* obj);
        visit_status_t pre_visit(meta *parent, const char* name,
                                 meta *obj, int depth);
        visit_status_t post_visit(meta *parent, const char* name,
                                  meta *obj, int depth);
    };

    // id -> object pointer map using default comapre functor
    // less<int>. use map instead of hash_map so the objects
    // are always sorted by their id
    typedef std::map<int, meta*> obj_map_t;

public:
    // next avaiable FS object id.  we no longer guarentee
    // this.  [we guarentee if object B is created after
    // objecte A, then B's id is greater than A's id ]
    int _id; 

    // next file system object name.  always increase when a
    // new name is introuduced into the file system. It's
    // different from _id because a unique object may have
    // multiple names.  

    // we can't guarentee that if _id1 < _id2, then _id1's
    // smallest name _name_id1 < _id2's smallest name
    // _name_id2.  dir entries can be removed/renamed!!
    int _name_id; 

	
    // map for all the file system objects. _objs[0] is
    // always the root, i.e. &_root;
    obj_map_t _objs;

    dir _root; // every file system has a root
    std::string _mnt; // mount point. it has the leading '/'

    // simple statistics: # of files, # of dirs, # of symlinks
    int _nr[ABST_MAX];

    typedef	Sgi::hash_map<ino_t, int> actual_to_abst_t;

    actual_to_abst_t actual_to_abst_map;
    FsTestDriverBase *_driver;

    void init(void);

    void add_new_obj(meta *o, int id);
    void add_new_obj(meta *o);
    void delete_obj(meta *o);

    void link_obj(meta *parent, int child_id, const char* name);
    void link_obj(meta *parent, meta *child, const char* name);
    void unlink_obj(meta *parent, const char* name);
    meta* get_child(meta *parent, const char* name);

    int dfs_(meta* parent, const char* name, 
                      meta* obj, int depth, visitor &v);

    void abstract_one(const char* paren_path,
                      const char* path,
                      actual_to_abst_t &);
    void abstract_dir(const char *path,
                      actual_to_abst_t &);
    void abstract_data(const char* path);

    // sanity checks
    bool detect_circle(void);
    bool detect_dangling(void);
    void check_objects(void);
    void internal_check(void);

    bool is_leaf(meta *o);

    //bool is_lost_and_found(meta*o);
    bool is_any(meta *o) {
        //return !is_lost_and_found(o);
        return true;
    }
    bool is_file(meta *o) {
        return o->type != ABST_DIR;
        //&& !is_lost_and_found(o);
    }
    bool is_reg_file(meta *o) {
        return o->type == ABST_FILE;
        //&& !is_lost_and_found(o);
    }
    bool is_dir(meta *o) {
        return o->type == ABST_DIR;
        //&& !is_lost_and_found(o);
    }
    bool is_empty_dir(meta *o) {
        return o->type == ABST_DIR
            && CAST(dir*, o)->entries.size() == 0;
        //&& !is_lost_and_found(o);
    }
    bool is_nonroot(meta *o) {
        return o->type != ABST_DIR 
            || CAST(dir*, o)->paren != NULL;
        //&& !is_lost_and_found(o);
    }


public:
    void clear(void);

    AbstFS() {init(); _driver = NULL;}
    AbstFS(FsTestDriverBase *driver) {init(); _driver = driver;}
    AbstFS(AbstFS &f);
    virtual ~AbstFS() {clear();}
    void copy(AbstFS &f);


    // only handle simple error cases.  name resolution is
    // usually done by VFS, which is not our main target
    int walk_path(const char* path, namei_t &nd);
    void find_back_path (meta *o, char* path, unsigned len);

    int creat(const char *pathname, mode_t mode);
    int link(const char *oldpath, const char* newpath);
    int rename(const char *oldpath, const char *newpath);
    int unlink (const char *pathname);
    int symlink(const char *oldpath, const char *newpath);

    int mkdir(const char *pathname, mode_t mode);
    int rmdir(const char *pathname);

    int truncate(const char *path, off_t length);
    int pread(const char *pathname, void* buf, size_t size, off_t);
    int pwrite(const char *pathname, const void *buf, size_t size, off_t);
#if 0
    void write_sig(const char* pathname, signature_t sig, 
                   off_t size, mode_t m);
    void write_sig(const char* pathname, signature_t sig, off_t size);
#endif
    void read_file(const char* pathname, signature_t& sig,
                   off_t &size, mode_t &m);
    int stat(const char *file_name, struct stat *buf);
    int stat(const char* path, meta *&o);

#if 0
    // TODO: the following file/symlink functions are not yet
    // implmeneted
    int readlink(const char *path, char *buf, size_t bufsiz);

    int chmod(const char *path, mode_t mode);
    int chown(const char *path, uid_t owner, gid_t group);
    int fchown(int fd, uid_t owner, gid_t group);
    int lchown(const char *path, uid_t owner, gid_t group);
    int fstat(int filedes, struct stat *buf);
    int lstat(const char *file_name, struct stat *buf);
    int ioctl(int d, int request, ...);
#endif

    virtual void mount_path(const char* mnt) { _mnt = mnt;}
    virtual void driver(FsTestDriverBase *driver) { _driver = driver;}
#if 0
    virtual int mount(const char *target, 
                      unsigned long mountflags, const void *data);

    int umount(const char *target);
    int umount2(const char *target, int flags);
#endif

    // this visit starts from the file system root, then
    // follows the directory entries to do a DFS traversal.
    // Directory entries are sorted based on strcmp
    int dfs(visitor &f);
    // returns dfs order of ids
    void dfs_order(id_map_t &order, bool pre);
    static void id_map_to_order(id_map_t& id_map, 
                                std::vector<int>& order);
    // id -> visited.  visited == INVALID_ID means not
    // visited, unattached
    void find_reachable(id_map_t &reachable);

    void remove_orphans(void);
    void fix_refcnts(void);

    void print(std::ostream &out) const;
    void print(void) const;

    struct delayed_link{
        int id;
        int child_id;
        std::string entry_name;
        delayed_link(int i, int ci, const char* name) {
            id = i;
            child_id = ci;
            entry_name = name;
        }
    };

    static void print_obj(const meta *obj, 
                          const char* name, std::ostream &out);
    static meta *new_obj(abst_file_type_t type, mode_t m);

    enum marshal_purpose {MARSHAL_ONLY, CANON_OBJ, CANON_METADATA};
    static void marshal_obj_helper(Archive &ar, meta *o, 
                                   enum marshal_purpose purpose, 
                                   id_map_t* id_map);

    static void marshal_data(Archive &ar, data &d);

    static void unmarshal_data(Archive &ar, data&);
    static void marshal_obj(Archive &ar, meta *o);

    static meta* unmarshal_obj(Archive &ar, 
                               std::vector<struct delayed_link>&);

    void marshal(Archive& ar);
    void marshal(char *&data, int &len);
    void unmarshal(Archive& ar);
    void unmarshal(const char *data, int len);

    // lossy marshal for comparison
    static signature_t data_sig(data& d);
    static void canon_data_region(Archive &ar, struct data_region *dr);
    static void canon_data(Archive &ar, data& d);
    void canon_helper(Archive& ar, enum marshal_purpose purpose);
    void canon_objects(Archive& ar);
    void canon_metadata(Archive& ar);

    void condense_ids(void);
    void dfs_remap_ids(void);
    void fix_ids(id_map_t& id_map);

    // TODO: should really separate marshal and hash.  marshal
    // can store all the ids, namei ids.  but when we compute
    // hash, we ignore all these ids, and just compute a
    // location-independent hash.  the remap id thing isn't a
    // good idea, as we need to remap everything.

    // This function computes a "content" signature for a file
    // system.  It only considers file content, file system
    // topology and # of files, symlinks and
    // directories. Names and content of symbolic links are
    // ignored.  Use this function to compute state sigature
    typedef void (*label_fn_t)(meta* o, char *label, int maxlen);
    signature_t get_sig(label_fn_t);
    signature_t obj_get_sig(unsigned obj_id);
    signature_t get_sig_no_name(void);
    // This computes a signature that incorperates names.  We
    // can compare two abstract file systems by comparing their
    // signatures returned by this function.  We don't need to
    // use the generic signature function to implement this
    // function, as the directory entries are already sorted
    // in strcmp order
    signature_t get_sig_with_name (void);
    signature_t get_metadata_sig_with_name(void);

    int compare(AbstFS& fs);
    int compare_metadata(AbstFS& fs);
    // given an inode number, compare the corresponding
    // objects in both file systems.  must called after
    // this->abstract and fs.abstract, so that visited is
    // initialized
    int compare_obj(AbstFS &fs, ino_t ino, int &exists);
    // compare the given file
    int compare_obj(AbstFS &fs, const char* file, int &exists);
    static void canon_obj(AbstFS::meta *o, char *label, int max_len);
    static int compare_obj(meta *o1, meta *o2);

    void abstract(const char* mntpoint);
    void remove_ignored(void);

    int choose_if(obj_pred_fn_t pred, choose_fn_t choose, 
                  char* path, unsigned len);
    int choose_any(char* path, unsigned len);
    int choose_file(char* path, unsigned len); // including symlinks
    int choose_reg_file(char* path, unsigned len); // regular files only
    int choose_dir(char* path, unsigned len);
    int choose_empty_dir(char* path, unsigned len);
    int choose_nonroot(char* path, unsigned len);
    void new_name(char* path, unsigned len);
    void new_path(char* path, unsigned len);

    obj_map_t* get_objs(void);
};

std::ostream& operator<<(std::ostream& o, const AbstFS& fs);
std::ostream& operator<<(std::ostream& o, const AbstFS::file& f);
#endif
