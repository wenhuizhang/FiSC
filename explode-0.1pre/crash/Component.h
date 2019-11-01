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


#ifndef __COMPONENT_H
#define __COMPONENT_H

#include<vector>
#include<set>
#include"hash_map_compat.h"
#include<string>
#include<assert.h>

#include "mcl.h"
#include "composit_iterator.h"
#include "NetLib.h"


enum fsck_mode {
    fsck_repair=0, // check and repair
    fsck_force_repair=1, // check and repair even if fs is clean
    fsck_label_badblock=2,  // look for bad blocks
    fsck_replay_only=3, // only replay the journal. valid only for JFS
    fsck_rebuild_tree=4, // rebuild tree. valid only for reiser
};

typedef std::vector<int> threads_t;

class Component;
typedef std::vector<Component*> component_vec_t;

// static configuration options for each system should be
// implmeneted in a subclass of Component
class Component {
public:

    typedef std::vector<Component*> container_type;

    friend class _Composite_pre_iterator<container_type>;
    friend class _Composite_post_iterator<container_type>;

    typedef _Composite_pre_iterator<container_type> iterator;
    typedef _Composite_post_iterator<container_type> post_iterator;


protected:
    std::string _path;
    HostLib* _ekm;
    int _size;
    bool _in_kern; // run in user space or kernel space.  needed
                   // by ctrl-c checking

    // set size to be the size of the device below
    virtual void set_default_size(void) {
        if(children.size() != 1) {
            // dunno how to set in this case. 
            abort();
        }
        if(!_size) // if size isn't set yet
            _size = children[0]->size();
    }

public:

    iterator begin()
    { return iterator(this, false); }

    iterator end()
    { return iterator(this, true); }

    post_iterator pbegin() 
    { return post_iterator(this, false); }

    post_iterator pend()
    { return post_iterator(this, true); }

    Component(const char* path, HostLib *ekm); 
    virtual ~Component(void) {}

    void plug_child(Component* c) {children.push_back(c);}

    // these functions don't need to be virtual
    // plug a component under this component
    const char* path(void) const {return _path.c_str();}
    bool in_kern(void) const {return _in_kern;}
    bool in_user(void) const {return !_in_kern;}

    int size(void) const {return _size;}
    void size(int s) {_size = s;}
    HostLib *ekm(void) const {return _ekm;}


    container_type children;

    // methods that operate only on this storage component.  They
    // are not recursive, i.e., they won't do anything to the
    // children of this storage component.  User over-ride these
    // to provide their own storage components
    virtual void init(void) {}
    virtual void mount(void) {}
    virtual void umount(void) {}
    virtual void recover(void) {}
    virtual void threads(threads_t&) {}

public:
    // methods that operate on the stack of components.
    void init_all(void);
    void mount_all(void);
    void umount_all(void);
    void recover_all(void);
    void threads_all(threads_t& th);

    void mount_user(void);
    void umount_user(void);
    void recover_user(void);

    void mount_kern(void);
    void umount_kern(void);
    void recover_kern(void);

    static int get_pid(const char* name);
};
#endif
