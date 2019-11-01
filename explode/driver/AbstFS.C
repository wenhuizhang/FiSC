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


/* TODO  think about id.  make it more clear */
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>

#include <list>
#include <algorithm>

#include "AbstFS.h"

#include "CrashSim.h"
#include "FsTestDriver.h"
#include "TreeCanon.h"
#include "bisim.h"
#include "lookup2.h"
#include "data.h"
#include "writes.h"
#include "driver-options.h"

//#define DEBUG_ABSTFS
#ifdef DEBUG_ABSTFS
void xi_assert_failed(const char* file, 
                           int line, const char* fmt, ...)
{
    assert(0);
}
int xe_random (int ceil) {
    return random()%ceil;
}
#endif

#define RETURN(ret) do{xi_assert(ret>=0, ""); return ret;}while(0)

AbstFS::namei_t AbstFS::namei_init = {0, NULL, {0,},};
AbstFS::namei_t AbstFS::parent_namei_init = {1, NULL, {0,},};

using namespace std;

void AbstFS::dir::add_entry(const char* name, int id) {
    entry_map_t::iterator it = entries.find(name);
    xi_assert(it == entries.end(), "entry already exists");
    char *key = xstrdup(name);
    assert(id > 0);
    entries[key] = id;
}

int AbstFS::dir::rm_entry(const char* name) {
    entry_map_t::iterator it = entries.find(name);
    xi_assert (it != entries.end(), "entry doesn't exist");
    const char *key = (*it).first;
    int objid = (*it).second;
    entries.erase(it);
    delete [] key;
    return objid;
}

int AbstFS::dir::get_entry(const char* name) {
    entry_map_t::iterator it;
    it = entries.find(name);
    //xi_assert (it != entries.end(), "entry doesn't exist");
    if(it == entries.end())
        return INVALID_ID;

    return (*it).second;
}

void AbstFS::file::print(ostream &o)
{
    o << "[" << type_char[type] << ":" 
      << "mode=" << oct << mode << dec << ":"
      << ":size=" << size
      << ":sig= " << sig
      << "]\n";
}

size_t AbstFS::file::pwrite(const void *buf, 
                            size_t size, off_t off)
{
    if(this->size < (off_t)(off+size))
        this->size = off + size;

    d.pwrite((char*)buf, size, off);
    // FIXME a nice way to represent data.
    this->sig = data_sig(d);

    return size;
}

size_t AbstFS::file::pread(void *buf, size_t size, off_t off)
{
    size_t avail_size;
    if(off >= this->size)
        avail_size = 0;
    else if(this->size < (off_t)(off+size))
        avail_size = this->size - off;
    else
        avail_size = size;

    if(avail_size)
        d.pread(buf, avail_size, off);

    return avail_size;
}

int AbstFS::file::truncate(off_t length)
{
    this->size = length;

    d.truncate(length);
    this->sig = data_sig(d);

    return 0;
}

AbstFS::meta *AbstFS::new_obj(abst_file_type_t type, mode_t m)
{
    meta *o;
    switch(type) {
    case ABST_FILE:
        o = new file(m);
        break;
    case ABST_DIR:
        o = new dir(m);
        break;
    case ABST_SYM_LINK:
        o = new sym_link();
        break;
    default:
        assert(0);
    }
    assert(o);
    o->id = INVALID_ID; // invalid
    return o;
}

void AbstFS::print_obj(const meta *obj, const char* name, std::ostream &out)
{
    out << "[" << type_char[obj->type] << ":" 
        << "id=" << obj->id << ":"
        << "mode=" << oct << obj->mode << dec << ":"
        << "ref=" << obj->ref << ":"
        << "ptr=" << obj;
    if(name)
        out << ":name=" << name;
    const file *f = NULL;
    const sym_link *l = NULL;
    const dir *d = NULL;
    switch (obj->type) {
    case ABST_FILE:
        f = CAST(const file*, obj);
        out << ":size=" << f->size
            << ":sig=" << f->sig
            << ":data=" << f->d;
        break;
    case ABST_DIR:
        d = CAST(const dir*, obj);
        out << ":#children=" << d->entries.size();
        break;
    case ABST_SYM_LINK:
        l = CAST(const sym_link*, obj);
        out << ":link=" << l->path;
        break;
    default:
        break;
    }
    out << "]\n";
}

AbstFS::visit_status_t 
AbstFS::printer::pre_visit(meta* parent, const char* name, 
                           meta* obj, int depth)
{
    if (parent == NULL) {
        (*o) << "[d:0:root:" << obj << "]\n";
        return VISIT_CONT;
    }
    while (depth--)	(*o) << "        ";

    if(!obj) {
        (*o) <<"[id="<<CAST(dir*, parent)->get_entry(name)<<":"
             <<"name=" << name << ":<dangling>]\n";
        return VISIT_CONT;
    } else 
        print_obj(obj, name, *o);

    return VISIT_CONT;
}


void AbstFS::visit_order::add_to_order(meta *obj)
{
    // must initialize to INVALID_ID
    assert(order->find(obj->id) != order->end());
    if((*order)[obj->id] != INVALID_ID)
        return; // already visited

    if(obj->id == 0 && pre)
        assert(cur == 0);

    (*order)[obj->id] = cur++;
}

AbstFS::visit_status_t 
AbstFS::visit_order::pre_visit(meta* parent, const char* name, 
                               meta* obj, int depth)
{
    if(pre)
        add_to_order(obj);
    return VISIT_CONT;
}

AbstFS::visit_status_t 
AbstFS::visit_order::post_visit(meta* parent, const char* name, 
                                meta* obj, int depth)
{
    if(!pre)
        add_to_order(obj);
    return VISIT_CONT;
}

const char AbstFS::type_char[ABST_MAX] = {'f', 'd', 'l'};

void AbstFS::init(void) {
    _id = 1;
    _name_id = 1;
    memset ((void*)_nr, 0, sizeof(_nr));
    _root.id = 0;
    _objs[0] = &_root;
}

AbstFS::AbstFS(AbstFS &f) {
    copy(f);
}

void AbstFS::copy(AbstFS &f)
{
    Archive ar;
    clear();
#if 1
    f.marshal(ar);
    unmarshal(ar);
#else
    int i;
    for(i = 0; i < (int)f._objs.size(); i++) {
        meta *o = f._objs[i]->clone();
        _objs[o->id] = o;
    }
#endif
}

void AbstFS::clear(void) {
    // we don't delete root here, because even an empty fs has a root
    actual_to_abst_map.clear();

    obj_map_t::iterator it;
    _objs.erase(0);
    while (_objs.size()) {
        it = _objs.begin();
        delete (*it).second;
        _objs.erase(it);
    }
    _root.clear();
    init();
}
	

/* this messy function parses/walks a given path name. It doesn't
   handle all the error cases.

   we require that
   1. path names have a leading '/' 
   2. no ending '/'
   3. no double '/'
   examples: "/dir1/dir2/file3", "/file", "/" 

   todo: handle symbolic links. handle relative paths
*/
int AbstFS::walk_path(const char* path, namei_t &nd)
{
    char path_tmp[MAX_PATH] = {0};
    char name_tmp[MAX_NAME] = {0};
    int len = strlen(path);
    int id;

    xi_assert(len > 0 && len < MAX_PATH, 
                   "invalid path length");
    xi_assert (path[0] == '/', "only absolute path");
    xi_assert(path[len-1] != '/', "path can't end with /");

    xi_assert (!strncmp(path, _mnt.c_str(), strlen(_mnt.c_str())), "root names don't match");
    path += strlen(_mnt.c_str());

    strcpy(path_tmp, path);

    char *p, *q, *end;
    if (nd.want_parent) {
        p = strrchr(path_tmp, '/');
        xi_assert(p, "we use absolute path, "\
                       "at least there is a leading /");
        *(p++) = '\0';
        int len2 = strlen(p);
        xi_assert(len2 > 0 && len2 < MAX_NAME,
                       "invalid name length");
        memcpy(nd.name, p, len2);
    }

    p = path_tmp+1; // +1 to skip over the leading '/'
    end = path_tmp + strlen(path_tmp);
    nd.obj = &_root;
    dir *o = &_root;
    while(p < end) {
        xi_assert (o, "path can't be fully parsed");
        q = strchr(p+1, '/');
        int len3 = ((q==NULL)? strlen(p) : (q-p));
        xi_assert(len3 > 0 && len3 < MAX_NAME,
                       "invalid name length");
        memcpy(name_tmp, p, len3);
        name_tmp[len3] = '\0';
        p += len3+1; // +1 to skip over the next '/'
        id = o->get_entry(name_tmp);
        nd.obj = (id < 0)? NULL:_objs[id];
        if(!nd.obj)	break; // exit loop if obj not found

        if (nd.obj->type == ABST_SYM_LINK) {
            // handle symbolic links here
        }

        // ok if nd.obj is a file. we'll assert(o) next iteration
        o = CAST(dir*, nd.obj); 
    }

    // found the object
    if(nd.obj)
        return 0;
	
    return -1;
}

void AbstFS::add_new_obj(meta *o, int id)
{
    _nr[o->type]++;
    o->id = id;
    _objs[id++] = o;
    internal_check();

}
void AbstFS::add_new_obj(meta *o)
{
    add_new_obj(o, _id);
    _id ++;
}

void AbstFS::delete_obj(meta *o)
{
    _nr[o->type]--;
    _objs.erase(o->id);

    if(o->type == ABST_DIR) {
	dir *d = CAST(dir*, o);
	assert(d);

        entry_map_t::iterator it;
        for(it = d->entries.begin(); it != d->entries.end(); it++)
            unlink_obj(d, (*it).first);
    }

    delete o;
}

void AbstFS::link_obj(meta *paren, meta *child, const char* name)
{
    dir* parent = CAST(dir*, paren);
    parent->add_entry(name, child->id);
    child->ref ++;
}

void AbstFS::link_obj(meta *paren, int child_id, const char* name)
{
    dir* parent = CAST(dir*, paren);
    parent->add_entry(name, child_id);
    if(_objs.find(child_id) != _objs.end())
        _objs[child_id]->ref ++;
}

void AbstFS::unlink_obj(meta *paren, const char* name)
{
    dir* parent = CAST(dir*, paren);
    int id = parent->rm_entry(name);
    assert(id >= 0);
    if(_objs.find(id) == _objs.end())
        return;
    meta* child = _objs[id];
    child->ref --;
    if (child->ref == 0)
        delete_obj(child);
}

AbstFS::meta* AbstFS::get_child(meta *paren, const char* name)
{
    dir* parent = CAST(dir*, paren);
    int id = parent->get_entry(name);
    assert(id >= 0);
    return _objs[id];
}

int AbstFS::creat(const char *path, mode_t mode)
{
    namei_t nd = parent_namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    file *child = new file(mode);
    assert(child);
    add_new_obj(child);
    link_obj(nd.obj, child, nd.name);
    RETURN(ret);
}

int AbstFS::link(const char *oldpath, const char* newpath)
{
    namei_t nd = namei_init;
    int ret = walk_path (oldpath, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed",oldpath);
    namei_t nd_new = parent_namei_init;
    ret = walk_path (newpath, nd_new);
    xi_assert(ret >= 0, "walk_path (%s) failed", newpath);
    link_obj (nd_new.obj, nd.obj, nd_new.name);
    RETURN(ret);
}

int AbstFS::rename(const char *oldpath, const char* newpath)
{
    namei_t nd_old = parent_namei_init;
    int ret = walk_path (oldpath, nd_old);
    xi_assert(ret >= 0, "walk_path (%s) failed", oldpath);
    meta *old_obj = get_child (nd_old.obj, nd_old.name);

    namei_t nd_new = parent_namei_init;
    ret = walk_path (newpath, nd_new);
    xi_assert(ret >= 0, "walk_path (%s) failed", newpath);
	
    link_obj(nd_new.obj, old_obj, nd_new.name);
    unlink_obj(nd_old.obj, nd_old.name);

    RETURN(ret);
}

int AbstFS::unlink(const char* path)
{
    namei_t nd = parent_namei_init;
    int ret = walk_path (path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_DIR, 
                    "unlink parent must be a directory");
    unlink_obj (nd.obj, nd.name);
    RETURN(ret);
}

int AbstFS::symlink(const char *oldpath, const char *newpath)
{
    namei_t nd = parent_namei_init;
    int ret = walk_path (newpath, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", newpath);
    meta *child = new sym_link(oldpath);
    assert(child);
    add_new_obj(child);
    link_obj(nd.obj, child, nd.name);
    RETURN(ret);
}

int AbstFS::mkdir(const char* path, mode_t mode)
{
    namei_t nd = parent_namei_init;
    int ret = walk_path (path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    meta *child = new dir(mode);
    assert(child);
    add_new_obj(child);
    link_obj (nd.obj, child, nd.name);
    CAST(dir*, child)->set_paren(CAST(dir*, nd.obj));
    RETURN(ret);
}

int AbstFS::rmdir(const char *path)
{
    namei_t nd = parent_namei_init;
    int ret = walk_path (path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_DIR, 
                    "rmdir parent must be a directory");
    dir *paren = CAST(dir*, nd.obj);
    dir *child = CAST(dir*, _objs[paren->get_entry(nd.name)]);
    xi_assert(child && child->entries.size() == 0, 
                   "rmdir must be called on an empty dir");
    unlink_obj (nd.obj, nd.name);
    RETURN(ret);
}

int AbstFS::stat(const char* path, meta *& o)
{
    namei_t nd = namei_init;
    int ret = walk_path (path, nd);
    //xi_assert(ret >= 0, "walk_path (%s) failed", path);
    o = nd.obj;
    return ret;
}

int AbstFS::truncate(const char *path, off_t length)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_FILE, 
                    "truncate must be called on a file");
    file *f = CAST(file*, nd.obj);
    return f->truncate(length);
}

#if 0
void AbstFS::write_sig(const char* path, signature_t sig, off_t size, mode_t m)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_FILE, 
                    "truncate must be called on a file");
    file *f = CAST(file*, nd.obj);
    f->sig = sig;
    f->size = size;
    f->mode = m;
}

void AbstFS::write_sig(const char* path, signature_t sig, off_t size)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_FILE, 
                    "truncate must be called on a file");
    file *f = CAST(file*, nd.obj);
    f->sig = sig;
    f->size = size;
}

#endif

void AbstFS::read_file(const char* path, signature_t& sig, 
                       off_t &size, mode_t &m)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_FILE, 
                    "truncate must be called on a file");
    file *f = CAST(file*, nd.obj);
    sig = f->sig;
    size = f->size;
    m = f->mode;
}

int AbstFS::pwrite(const char* path, const void *buf, 
                   size_t size, off_t off)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_FILE, 
                    "truncate must be called on a file");
    file *f = CAST(file*, nd.obj);
    return f->pwrite(buf, size, off);
}

int AbstFS::stat(const char* path, struct stat *st)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    //xi_assert(ret >= 0, "walk_path (%s) failed", path);
    if(ret < 0)
        return ret;

    st->st_ino = nd.obj->id;
    st->st_mode = nd.obj->mode;
    st->st_size = 0;
    if(nd.obj->type == ABST_FILE)
        st->st_size = CAST(file*, nd.obj)->size;
    return ret;
}

int AbstFS::pread(const char *path, void* buf, 
		  size_t size, off_t off)
{
    namei_t nd = namei_init;
    int ret = walk_path(path, nd);
    xi_assert(ret >= 0, "walk_path (%s) failed", path);
    xi_assert (nd.obj->type == ABST_FILE, 
                    "truncate must be called on a file");
    file *f = CAST(file*, nd.obj);
    return f->pread(buf, size, off);
}

#if 0
int AbstFS::mount(const char *target, 
		  unsigned long mountflags, const void *data)
{
    xi_assert(strlen(target) < MAX_PATH, "mount target is longer than MAX_PATH");
    _mnt = target;
    clear();
    return 0;
}

int AbstFS::umount(const char *target)
{
    _mnt = "";
    return 0;
}

int AbstFS::umount2(const char *target, int flags)
{
    return umount(target);
}
#endif

int AbstFS::choose_if(obj_pred_fn_t pred, choose_fn_t choose,
                      char* path, unsigned path_len)
{
    vector<meta *> qualified;
    unsigned i;
    // don't use count_if and find_if. If we use them it'll be O(n^2)
    obj_map_t::iterator it;
    for (it = _objs.begin(); it != _objs.end(); it++) {
        if ((this->*pred)((*it).second))
            qualified.push_back((*it).second);
    }

    // no qualified objects found
    if (!qualified.size())
        return -1;

    i = choose (qualified.size());
    xi_assert(0<=i && i<qualified.size(), "something wrong with xe_random");
    meta *o = qualified[i];

    // "choose" one path ouf of all paths to o
    find_back_path (o, path, path_len);
    vvv_out << " choose_if returns " << path << endl;
    return 0;
}

#if 0
// with actual_fs->ignore, this is useless
bool AbstFS::is_lost_and_found(AbstFS::meta *o)
{
    char path[MAX_PATH];
    find_back_path(o, path, sizeof(path));
    return strstr(path, "lost+found") || strstr(path, "fsck");
}
#endif

#if 0
bool is_lost_and_found(AbstFS::meta *o) 
{
}
bool is_any(AbstFS::meta *o) {return true;}
bool is_file(AbstFS::meta *o){return o->type != AbstFS::ABST_DIR;}
bool is_reg_file(AbstFS::meta *o){return o->type == AbstFS::ABST_FILE;}
bool is_dir(AbstFS::meta *o){return o->type == AbstFS::ABST_DIR;}
bool is_empty_dir(AbstFS::meta *o)
{
    return o->type == AbstFS::ABST_DIR
        && CAST(AbstFS::dir*, o)->entries.size() == 0;
}
bool is_nonroot(AbstFS::meta *o) 
{
    return o->type != AbstFS::ABST_DIR 
        || CAST(AbstFS::dir*, o)->paren != NULL;
}
#endif

int AbstFS::choose_any(char* path, unsigned len)
{
    return choose_if(&AbstFS::is_any, xe_random, path, len);
}

int AbstFS::choose_file(char* path, unsigned len)
{
    return choose_if(&AbstFS::is_file, xe_random, path, len);
}

int AbstFS::choose_reg_file(char* path, unsigned len)
{
    return choose_if(&AbstFS::is_reg_file, xe_random, path, len);
}

int AbstFS::choose_dir(char* path, unsigned len)
{
    return choose_if(&AbstFS::is_dir, xe_random, path, len);
}

int AbstFS::choose_empty_dir(char* path, unsigned len)
{
    return choose_if(&AbstFS::is_empty_dir, xe_random, path, len);
}

int AbstFS::choose_nonroot(char* path, unsigned len)
{
    return choose_if(&AbstFS::is_nonroot, xe_random, path, len);
}

static void id_to_name (int name_id, char* name, unsigned len)
{
    char tmp[256];
    sprintf(tmp, "%d", name_id);
    memset(name, '0', len);
    strcpy(name + (len-1) - strlen(tmp), tmp);
}

static int name_to_id (const char* name)
{
    if(!name) return 0;
    return atoi(name);
}

void AbstFS::new_name(char* name, unsigned name_len)
{
    // fill name with this id
#if 0
    id_to_name (_name_id++, name, 8);
#else
    id_to_name (_name_id++, name, name_len);
#endif
}

void AbstFS::new_path(char* path, unsigned path_len)
{
    char name_tmp[MAX_NAME] = {0};
    choose_dir(path, path_len);
    assert(get_option(fstest, name_len) <= (int)(sizeof name_tmp));
    new_name(name_tmp, get_option(fstest, name_len));

    xi_assert((unsigned)path_len > 1 + strlen(path) + 
                   strlen(name_tmp), "name too long (%s, %s)!", 
                   path, name_tmp);
    strcat(path, "/");
    strcat(path, name_tmp);
}

// can be made faster if it becomes a problem
void AbstFS::find_back_path (meta *o, char* path, unsigned path_len)
{
    class tracer: public visitor {
    public:
        list<const char *> path;
        meta *target;
    public:
        tracer(meta *o) {target = o;}
        visit_status_t pre_visit(meta* parent, const char* name, 
                                 meta* obj, int depth) {
            path.push_back(name);
            if (obj == target)
                return VISIT_STOP;
            return VISIT_CONT;
        }
        visit_status_t post_visit(meta* parent, const char* name, 
                                  meta* obj, int depth) {
            path.pop_back();
            return VISIT_CONT;
        }
    };
	
    tracer t(o);
    dfs(t);
    list<const char *>::iterator it;
    for (it = t.path.begin(); it != t.path.end(); it++) {
        const char* n = (*it);
        if (!n)
            strcpy(path, _mnt.c_str());
        else {
            xi_assert(strlen(path) + 1 + strlen(n) < path_len, "back path too long");
            strcat(path, "/");
            strcat(path, n);
        }
    }
}

#define CHECK_CONTINUE(x) if(x == VISIT_STOP) return x;

int AbstFS::dfs_(meta* parent, const char* name, 
                 meta* obj, int depth, visitor &v)
{
    int ret;	
    if(depth >= MAX_DEPTH) throw (InvalidFS("recursion too deep"));

    if(!obj) {
        if(!v.want_dangling)
            return VISIT_CONT;
        ret = v.pre_visit(parent, name, NULL, depth);
        CHECK_CONTINUE(ret);
        ret = v.post_visit(parent, name, NULL, depth);
        return ret;
    }

    ret = v.pre_visit(parent, name, obj, depth);
    CHECK_CONTINUE(ret);
    if (obj->type == ABST_DIR) {
        dir *d = CAST(dir*, obj);
        assert(d);
        entry_map_t::iterator it;
        for (it = d->entries.begin(); 
             it != d->entries.end(); it++) {
            int child_id = (*it).second;
            meta *child = NULL;
            if(_objs.find(child_id) != _objs.end())
                child = _objs[child_id];
            ret = dfs_(obj, (*it).first, child, depth+1, v);
            CHECK_CONTINUE(ret);
        }
    }
    v.post_visit(parent, name, obj, depth);
    return ret;
}
#undef CHECK_CONTINUE

int AbstFS::dfs(visitor &v)
{
    return dfs_ (NULL, NULL, &_root, 0, v); 
}

void AbstFS::dfs_order(id_map_t &order, bool pre)
{
    obj_map_t::iterator it;
    for(it = _objs.begin(); it != _objs.end(); it++)
        order[(*it).first] = INVALID_ID;

    visit_order v(&order, pre);
    dfs(v);
}

void AbstFS::id_map_to_order(id_map_t& id_map, vector<int>& order)
{
    int max = 0, i;
    id_map_t::iterator it;
    order.resize(id_map.size(), INVALID_ID);

    for(it = id_map.begin(); it != id_map.end(); it++) {
        if((*it).second != INVALID_ID) {
            // shouldn't visit a node twice
            assert(order[(*it).second] == INVALID_ID);
            order[(*it).second] = (*it).first;
            if(max < (*it).second)
                max = (*it).second;
        }
    }

    for(i = 0; i <= max; i ++)
        assert(order[i] >= 0);
    for(i = max+1; i < (int)id_map.size(); i++)
        assert(order[i] == INVALID_ID);

    order.resize(max+1);
}

void AbstFS::find_reachable(id_map_t &reachable)
{
    dfs_order(reachable, true);
}

bool AbstFS::detect_circle(void)
{
    class circle_detector: public visitor {
    public:
        bool has_circle;
        vector<int> path;
		
        circle_detector() {has_circle = false;}
        visit_status_t pre_visit(meta *parent, const char* name,
                                 meta *obj, int depth) {
            int i;
            for(i = path.size()-1; i >= 0; i--)
                if(obj->id == path[i]) {
                    has_circle = true;
                    return VISIT_STOP;
                }
            path.push_back(obj->id);
            return VISIT_CONT;
        }
        visit_status_t post_visit(meta *parent, const char* name,
                                  meta *obj, int depth) {
            path.pop_back();
            return VISIT_CONT;
        }
    };
	
    circle_detector d;
    dfs(d);
    return d.has_circle;
}

bool AbstFS::detect_dangling(void)
{
    class dangle_detector: public visitor {
    public:
        bool dangling;
        dangle_detector() {dangling = false; want_dangling = true;}
        visit_status_t pre_visit(meta *parent, const char* name,
                                 meta *obj, int depth) {
            if(!obj) {
                // must be a dangling entry 
                dangling = true;
                return VISIT_STOP;
            }
            return VISIT_CONT;
        }
    };
	
    dangle_detector d;
    dfs(d);
    return d.dangling;
}

void AbstFS::remove_orphans(void)
{
    id_map_t reachable;
    bool deleted;
    obj_map_t::reverse_iterator rit;

    find_reachable(reachable);

    do {
        deleted = false;
        rit = _objs.rbegin();
        while(rit != _objs.rend()) {
            if(reachable.find((*rit).first) != reachable.end() 
               && reachable[(*rit).first] == INVALID_ID) {
                deleted = true;
                delete_obj((*rit).second);
                reachable.erase((*rit).first);
                break;
            }
            rit++;
        }
    } while(deleted);
}

void AbstFS::fix_refcnts(void)
{
    map<int, int> refcnts;
    obj_map_t::iterator it;
    entry_map_t::iterator eit;
    for(it = _objs.begin(); it != _objs.end(); it++) {
        if(!is_dir((*it).second))
            continue;
		
        dir *d = CAST(dir*, (*it).second);
        for(eit = d->entries.begin(); eit != d->entries.end(); eit++) {
            int child_id = (*eit).second;
            if(refcnts.find(child_id) == refcnts.end())
                refcnts[child_id] = 1;
            else
                refcnts[child_id] ++;
        }
    }

    for(it = _objs.begin(); it != _objs.end(); it++) {
        if((*it).first == 0)
            assert(refcnts.find((*it).first) == refcnts.end());
        else {
            if(refcnts.find((*it).first) == refcnts.end())
                continue;

            assert(refcnts[(*it).first] > 0);
            if((*it).second->ref != refcnts[(*it).first])
                (*it).second->ref = refcnts[(*it).first];
        }
    }
}

void AbstFS::print(std::ostream& out) const
{
    id_map_t reachable;
    id_map_t::iterator it;

    cout << _nr[ABST_FILE] << " file(s), "
         << _nr[ABST_DIR]+1 << " dir(s), "
         << _nr[ABST_SYM_LINK] << " symlink(s), "
         <<_objs.size() << " total including unreachable\n"
         << "(next id=" << _id << ", "
         << "next name id="<<_name_id << ", "
         << "mnt=" << _mnt << ")\n";

#if 0
    cout << "id = " << _id << "  "
         << "name_id = " << _name_id << "\n";
#endif

    printer p(out);
    ((AbstFS*)this)->dfs(p);

    ((AbstFS*)this)->find_reachable(reachable);
    for(it = reachable.begin(); it != reachable.end(); it++)
        if((*it).second == INVALID_ID)
            print_obj(((AbstFS*)this)->_objs[(*it).first], 
                      "<unreachable>", out);
}

void AbstFS::print(void) const
{
    print(cout);
}

ostream& operator<<(ostream& o, const AbstFS& fs)
{
    fs.print(o);
    return o;
}

ostream& operator<<(ostream& o, const AbstFS::file& f)
{
    AbstFS::print_obj(&f, NULL, o);
    return o;
}

void AbstFS::marshal_data(Archive& ar, data& d)
{
    data::iterator it;
    ar << d.size();
    for(it = d.begin(); it != d.end(); it++) {
        struct data_region *dr = (*it).second;
        ar << dr->start
           << dr->end
           << dr->magic;

        if(!dr->magic)
            ar.put(dr->data, dr->end - dr->start);
    }
}

void AbstFS::unmarshal_data(Archive& ar, data& d)
{
    int n, i;
    struct data_region *dr;
    ar >> n;
    for(i = 0; i < n; i ++) {
        dr = new data_region;
        assert(dr);
        ar >> dr->start
           >> dr->end
           >> dr->magic;

        if(!dr->magic) {
            dr->data = new char[dr->end - dr->start];
            assert(dr->data);
            ar.put(dr->data, dr->end - dr->start);
        } else dr->data = NULL;

        d[dr->start] = dr;
    }
}

void AbstFS::marshal_obj(Archive& ar, meta *o)
{
    marshal_obj_helper(ar, o, MARSHAL_ONLY, NULL);
}

void AbstFS::marshal_obj_helper(Archive& ar, meta *o, 
                                enum marshal_purpose purpose, 
                                id_map_t* id_map)
{
    file *f = NULL;
    sym_link *l = NULL;
    dir *d = NULL;
    entry_map_t::iterator it;
    id_map_t::iterator id_it;
    int len, id, child_id;

    if(purpose == MARSHAL_ONLY)
        assert(id_map == NULL);
    else
        assert(id_map);

    ar << o->type;
    if(id_map) {
        assert(id_map->find(o->id) != id_map->end());
        // shouldn't be orphan
        assert((*id_map)[o->id] != INVALID_ID);
        id = (*id_map)[o->id];
    } else
        id = o->id;
    ar << id;
    ar << o->ref << o->mode;

    switch (o->type) {
    case ABST_FILE:
        if(purpose == CANON_METADATA)
            break;
        f = CAST(file*, o);
        ar << f->size
           << f->sig;
        if(purpose == MARSHAL_ONLY)
            marshal_data(ar, f->d);
        else
            canon_data(ar, f->d);
        break;
    case ABST_DIR:
        d = CAST(dir*, o);
        ar << d->entries.size();
        for (it = d->entries.begin();
             it != d->entries.end(); it++) {
            const char *ent_name = (*it).first;
            int len = strlen(ent_name);
            ar << len;
            ar.put(ent_name, len);
            child_id = (*it).second;
            if(id_map) {
                // child shouldn't be dangling
                assert(id_map->find(child_id) != id_map->end());
                assert((*id_map)[child_id] != INVALID_ID);
                child_id = (*id_map)[child_id];
            }
            ar << child_id;
        }
        break;
    case ABST_SYM_LINK:
        l = CAST(sym_link*, o);
        len = strlen(l->path);
        ar << len;
        ar.put(l->path, len);
        break;
    default:
        break;
    }
}
// TODO: rewrite this.  have a bool param (remap_id), start from
// root, follow names.  then we can use this to compare to
// abstract file system
void AbstFS::marshal(Archive& ar)
{
    internal_check();
    // marshal FS globals
    ar << _id
       << _name_id
       << _objs.size();
    ar.put(_mnt);

    for(int i = 0; i < ABST_MAX; i++)
        ar << _nr[i];
#if 0
    // not needed.  we have delayed links now
	
    // marshal all the objects. We do it backwards so when we
    // unmarshal we don't need to handle dangling object
    // pointers (directed acyclic graph).
    obj_map_t::reverse_iterator it;
    for(it = _objs.rbegin(); it != _objs.rend(); it++)
        marshal_obj(ar, (*it).second);
#else
    obj_map_t::iterator it;
    for(it = _objs.begin(); it != _objs.end(); it++)
        marshal_obj(ar, (*it).second);
#endif
}

signature_t AbstFS::data_sig(data& d)
{
    Archive ar;
    canon_data(ar, d);
    // cout << "data sig = " << ar.get_sig() << "\n";
    return ar.get_sig();
}

// the problem: one data obj has two regions, r1, r2
//              the other has one region, r100000...0r2
//              must detect that they are equal
// solution: encode the buffer into <tag><len>[content]
//           where tag is either magic, zero, or other
const int magic = 2, zero = 0, other = 1;
void AbstFS::canon_data_region(Archive &ar, struct data_region *dr)
{
    if(dr->magic)
        ar << magic
           << (dr->end - dr->start)
           << dr->magic;
    else {
        char *prev, *p, *end;
        prev = p = dr->data;
        end = dr->data + dr->end - dr->start;

        // this takes care of zeroes inside data regions
        while(p < end) {
            while(*p && p < end) p ++;
            if(p - prev) {
                ar << other
                   << p - prev;
                ar.put(prev, p-prev);
            }
            prev = p;
            while(!*p && p < end) p ++;
            if(p - prev)
                ar << zero
                   << p - prev;
            prev = p;
        }
    }
}

void AbstFS::canon_data(Archive&ar, data& d)
{
    data::iterator it;
    off_t prev_end;

#if 0
    // no size.  size of data regions may diff.  see above notes
    // regarding zeroes
    ar << d.size();
    if(d.size()) {
        it = d.begin();
        ar << (*it).first; // offset of first non-zero word
    }
#endif

    prev_end = 0;
    for(it=d.begin(); it!=d.end(); it++) {
        // if first region starts from 0, we just add two redunant
        // words 0 0 here.  won't affect canonical form
        ar << zero << ((*it).first - prev_end);
        canon_data_region(ar, (*it).second);
        prev_end = (*it).second->end;
    }
}

void test_data_sig(void)
{
    int i;
    char buf[300];
    char buf2[300];
    data d1, d2, d3, d4;
    struct data_region *dr, *dr1, *dr2;
	
    for(i = 0; i < (int)sizeof buf; i ++)
        buf[i] = 25;

    for(i = 0; i < (int)sizeof buf; i ++)
        buf2[i] = 25;
    buf2[25] = 26;

    assert(AbstFS::data_sig(d1) == AbstFS::data_sig(d2));

    dr = new struct data_region(50, 100, 0, buf);
    assert(dr);
    dr2 = dr->clone();

    d1.add(dr);
    d2.add(dr2);
    assert(AbstFS::data_sig(d1) == AbstFS::data_sig(d2));

    d2.clear();
    assert(AbstFS::data_sig(d1) != AbstFS::data_sig(d2));

    dr2 = new struct data_region(50, 100, 0, buf2);
    d2.add(dr2);
    assert(AbstFS::data_sig(d1) != AbstFS::data_sig(d2));	

    d1.clear();
    assert(AbstFS::data_sig(d1) != AbstFS::data_sig(d2));

    d2.clear();
    assert(AbstFS::data_sig(d1) == AbstFS::data_sig(d2));

    dr = new struct data_region(50, 100, 0, buf);
    assert(dr);
    dr2 = dr->clone();
    d1.add(dr);
    d2.add(dr2);

    dr = new struct data_region(200, 300, 0, buf);
    assert(dr);
    dr2 = dr->clone();
    d1.add(dr);
    d2.add(dr2);
    assert(AbstFS::data_sig(d1) == AbstFS::data_sig(d2));

    d1.clear();
    d2.clear();

    // internal zeroes
    memset(buf+100, 0, 100);
    dr = new struct data_region(50, 150, 0, buf);
    dr1 = new struct data_region(250, 350, 0, buf + 200);
    d1.add(dr);
    d1.add(dr1);

    dr2 = new struct data_region(50, 350, 0, buf);
    d2.add(dr2);

    Archive ar1, ar2;
    AbstFS::canon_data(ar1, d1);
    AbstFS::canon_data(ar2, d2);
	
    assert(AbstFS::data_sig(d1) == AbstFS::data_sig(d2));
}

void AbstFS::canon_helper(Archive& ar, enum marshal_purpose purpose)
{
    id_map_t id_map;
    id_map_t::iterator it;
    vector<int> order;
    int i;

    internal_check();

    dfs_order(id_map, true);
    id_map_to_order(id_map, order);
	
    for(i = 0; i < (int)order.size(); i++) {
        assert(_objs.find(order[i]) != _objs.end());
        meta *o = _objs[order[i]];
        marshal_obj_helper(ar, o, purpose, &id_map);
    }
}

void AbstFS::canon_objects(Archive& ar)
{
    canon_helper(ar, CANON_OBJ);
}

void AbstFS::canon_metadata(Archive& ar)
{
    canon_helper(ar, CANON_METADATA);
}

void AbstFS::marshal(char *&data, int& len)
{
    Archive ar;
    marshal(ar);
    data = ar.detach(len);
}

AbstFS::meta *AbstFS::unmarshal_obj(Archive& ar, vector<struct delayed_link>& delayed_links)
{
    char name_tmp[MAX_NAME] = {0};
    abst_file_type_t type = ABST_MAX;
    int ref, id;
    mode_t mode;
    signature_t sig;
    off_t size;
    // can't use static cast here. we want the reference
    ar >> type >> id >> ref >> mode;
	
    meta *o;
    file *f;
    int len, j, num_entries, child_id;
    switch(type) {
    case ABST_FILE:
        ar >> size
           >> sig;
        f = new file(mode, size, sig);
        assert(f);
        unmarshal_data(ar, f->d);
        o = f;
        break;
    case ABST_DIR:
        //o = (id == 0)? &_root : new dir(mode);
        o = new dir(mode);
        assert(o);
        ar >> num_entries;
        for (j = 0; j < num_entries; j++) {
            ar >> len;
            ar.get(name_tmp, len);
            name_tmp[len] = 0; // NULL terminate this string
            ar >> child_id;
            // this assertion is false!
            /* 
               xi_assert (id < child_id, "parent id(%d) must be smaller than child id(%d)", id, child_id);
            */
            if(id == child_id)
                xi_panic("circle when unmarshaling fs");
            else
                delayed_links.push_back(delayed_link(id, child_id,name_tmp));
        }
        break;
    case ABST_SYM_LINK:
        ar >> len;
        o = new sym_link;
        assert(o);
        ar.get(CAST(sym_link*, o)->path, len);
        break;
    default:
        assert(0);
    }
    o->id = id;
    return o;
}

void AbstFS::unmarshal(Archive& ar)
{
    internal_check();
    clear();
    int num_objs, i;
    vector<struct delayed_link>::iterator it;
    vector<struct delayed_link> delayed_links;

    // unmarshal FS globals
    ar >> _id
       >> _name_id
       >> num_objs;
    ar.get(_mnt);
    for(i = 0; i < ABST_MAX; i++)
        ar >> _nr[i];

    // unmarshal all the objects
    for (i = 0; i < num_objs; i ++) {
        meta *o = unmarshal_obj(ar, delayed_links);
        if(o->id == 0) {
            _root.copy(o);
            delete o;
            o = &_root;
        } 
        assert(o->id >= 0);
        _objs[o->id] = o;
    }

    for(it = delayed_links.begin(); it != delayed_links.end(); it++)
        link_obj(_objs[(*it).id], (*it).child_id, (*it).entry_name.c_str());

    internal_check();	
#if 0	
    xi_assert (ar.eof(), "file system is unmarsalled but input byte stream is not fully consumed");
#endif
}

void AbstFS::unmarshal(const char *data, int len)
{
    Archive ar;
    ar.attach((char*)data, len);
    unmarshal(ar);
    ar.detach(len);
}

bool AbstFS::is_leaf(meta *o)
{
    return o->type != ABST_DIR
        || CAST(dir*, o)->entries.size() == 0;
}

/* the hash signature should include both metadata and data.  We
   don't hash on obj id (i.e. inode # in a real file system) for 2
   reasons: first, because they don't carry any information. only for
   internal use.  as long as all the metadata and data are there, the
   file system should be considered correct.  second, different file
   systems have different ways to assign inode numbers.  we can't
   model all of them.

   However, to perform FS-specific checks, we may need to model inode
   numbers. 

   How about file names?  we abstract them away, too. 

   n unique objects (i.e. inodes) -> [1-n]
   m unique edges (i.e. dir entries, <parent, name, child>) ->  <parent, child>

   TODO: should be bottom up. 
   file data + topology(meta data)
   cannonicalize names
*/
#if 0
signature_t void AbstFS::get_sig_no_name(void)
{
    // cannonicalize ids
    condense_ids();

    // lossy marshal
    // marshal FS globals
    Archive ar;
    ar << _objs.size();
    for(int i = 0; i < ABST_MAX; i++)
        ar << _nr[i];

    obj_map_t::reverse_iterator it;
    for(it = _objs.rbegin(); it != _objs.rend(); it++) {
        meta *o = (*it).second;
        ar << o->id << o->type << o->ref << o->mode;

        file *f = NULL;
        sym_link *l = NULL;
        dir *d = NULL;
        entry_map_t::iterator it;

        switch (o->type) {
        case ABST_FILE:
            f = CAST(file*, o);
            ar << f->size
               << f->sig;
            break;
        case ABST_DIR:
            d = CAST(dir*, o);
            ar << d->entries.size();
            for (it = d->entries.begin();
                 it != d->entries.end(); it++)
                // *no* names!
                ar << (*it).second->id;
            break;
        case ABST_SYM_LINK:
            // *no* symbolic link
            l = CAST(sym_link*, o);
            break;
        default:
            break;
        }
    }
    ar.get_sig(sig);
}
#elif 0

/* Tree cannonlicalization algorithm based on A. Aho, J. Hopcroft,
   and J. Ullman. The Design and Analysis of Computer
   Algorithms. Addison-Wesley, Reading MA, 1974.

   basic idea is that we compute a cannonicalized string for each
   node at leavel i, sort them, then do the same thing for nodes at
   level i-1, when we reach root we get the cannonicalized string
   for this tree
*/

#define MAX_LABEL (64)
typedef struct node_sig_s {
    int level;
    bool is_leaf;
    char label[MAX_LABEL];
    int ord;
} node_sig_t;

signature_t void AbstFS::get_sig_no_name(void)
{

    class leaf_labeller: public visitor {
    private:
        node_sig_t* node_sigs;
    public:
        int max_depth;
        leaf_labeller(node_sig_t *n) : node_sigs(n), max_depth(0) {}
        visit_status_t pre_visit(meta* parent, const char* name, 
                                 meta* obj, int depth) {
            if(max_depth < depth) max_depth = depth;
            node_sigs[obj->id].level = depth;
            dir *d = CAST(dir*, obj);
			
            if (obj->type != ABST_DIR
                || !CAST(dir*, obj)->entries.size())
                node_sigs[obj->id].is_leaf = true;
            if(node_sigs[obj->id].is_leaf) {
                assert(sizeof(node_sig_t) > sizeof(char) + sizeof(signature_t));
                node_sigs[obj->id].label[0] = type_char[obj->type];
                if(obj->type == ABST_FILE)
                    memset(&node_sigs[obj->id].label[1], CAST(file*, obj)->sig, sizeof(signature_t));
				
            }
            return VISIT_CONT;
        }
    };

    // condense ids
    condense_ids();

    node_sig_t *node_sigs = new node_sig_t[_objs.size()];
    assert(node_sigs);
    leaf_labeller ll(node_sigs);
    dfs(ll);

    vector<list<node_sig_t *> > node_lvls(NULL, ll.max_depth);
    assert(node_lvls);
    memset((char*)node_lvls, 0, sizeof(node_lvls[0]) * ll.max_depth);

    int i;
    for(i = 0; i < _objs.size(); i++) {
        int lvl = node_sigs[i].level = ll.max_depth - node_sigs[i].level;
        node_lvls[lvl].push_back(&node_sigs[i])
            }

    list<node_sig_t *>::iterator it;
    for(i = 1; i < ll.max_depth; i++) {
        for (it = node_lvls[i].begin();
             it != node_lvls[i].end(); it++) {
            node_sig_t *n = *it;
            if (n->is_leaf)	continue;
        }
    }
	
    delete [] node_sigs;
}
#else

signature_t AbstFS::get_sig(label_fn_t labeller)
{
    class translator: public visitor {
    public:
        label_fn_t l;
        TreeCanon tc;
        Sgi::hash_map<int, TreeCanon::node_t*> m;

        translator(label_fn_t lf): l(lf) {}
		
        visit_status_t pre_visit(meta* parent, 
                                 const char* name, 
                                 meta* obj, int depth) {
            char label[MAX_LABEL];
            l(obj, label, sizeof(label));
            m[obj->id] = tc.add(parent?m[parent->id]:NULL, label);
            return VISIT_CONT;
        }
    };

    translator s(labeller);
    dfs(s);

    char *form;
    int form_len;
    s.tc.canon_form(form, form_len);
    return explode_hash((ub1*)form, form_len);
}

// type matters, so we stick the type there.  TODO: A hash on the
// type char should be sufficient.
void content_fn(AbstFS::meta *o, char *label, int max_len)
{
    assert (1 + sizeof(signature_t) <= (unsigned)max_len);
    memset(label, 0, max_len);
    label[0] = AbstFS::type_char[o->type];
    if(o->type == AbstFS::ABST_FILE) {
        AbstFS::file * f= CAST(AbstFS::file*, o);
        memcpy(&label[1], &f->sig, sizeof(f->sig));
    }
}

signature_t AbstFS::get_sig_no_name(void)
{
    return get_sig(content_fn);
}
#endif

// TODO: we want to record names, file data in this case.  this
// should be a function get_sig_sub_tree, and we do the bottom up
// content hashing, like in sfsro.  don't throw away any details.
// we'll need to throw away object id tho.
// 
// this is retarded.  should separate marshal and hash!!!!
signature_t AbstFS::get_sig_with_name(void)
{
    //dfs_remap_ids();
    Archive ar;
    canon_objects(ar);
    return ar.get_sig();
}

signature_t AbstFS::get_metadata_sig_with_name(void)
{
    //dfs_remap_ids();
    Archive ar;
    canon_metadata(ar);
    return ar.get_sig();
}

void AbstFS::canon_obj(AbstFS::meta *o, char *label, int max_len)
{
    assert (1 + sizeof(signature_t) <= (unsigned) max_len);
    memset(label, 0, max_len);

    label[0] = AbstFS::type_char[o->type];

    AbstFS::file * f;
    AbstFS::dir *d;
    AbstFS::sym_link *l;
    signature_t sig = 0;
    AbstFS::entry_map_t::iterator it;
    switch (o->type) {
    case AbstFS::ABST_FILE:
        f= CAST(AbstFS::file*, o);
        sig = f->sig;
        break;
    case AbstFS::ABST_DIR:
        d = CAST(AbstFS::dir*, o);
        for(it = d->entries.begin(); 
            it != d->entries.end(); it++ ) {
            const char* const name = (*it).first;
            sig = hash3((ub1*)name, strlen(name)+1, sig);
        }
        break;
    case AbstFS::ABST_SYM_LINK:
        l = CAST(AbstFS::sym_link*, o);
        sig = hash3((ub1*)l->path, strlen(l->path)+1, 0);
        break;
    default:
        assert(0);
    }
    memcpy(&label[1], &sig, sizeof(sig));
}

// TODO: should compute a sig for subtree. this should be done
// just once. currently this is really really really stupid.
signature_t AbstFS::obj_get_sig(unsigned obj_id)
{
    class translator: public visitor {
    public:
        label_fn_t l;
        TreeCanon tc;
        Sgi::hash_map<int, TreeCanon::node_t*> m;

        translator(label_fn_t lf): l(lf) {}
		
        visit_status_t pre_visit(meta* parent, 
                                 const char* name, 
                                 meta* obj, int depth) {
            char label[MAX_LABEL];
            l(obj, label, sizeof(label));
            m[obj->id] = tc.add(parent?m[parent->id]:NULL, label);
            return VISIT_CONT;
        }
    };

    translator s(canon_obj);
    dfs(s);

    char *form;
    int form_len;
    s.tc.node_canon_form(s.m[obj_id], form, form_len);
    return explode_hash((ub1*)form, form_len);
}

int AbstFS::compare(AbstFS& fs)
{
    signature_t abst, actual;
    abst =	get_sig_with_name();
    actual = fs.get_sig_with_name();
    vvv_out <<"fs(" << abst << ") " 
            << ((abst==actual)? "==":"!=") 
            << " fs(" << actual << ")\n";
    return abst - actual;
}

int AbstFS::compare_metadata(AbstFS& fs)
{
    signature_t abst, actual;
    abst =	get_metadata_sig_with_name();
    actual = fs.get_metadata_sig_with_name();
    vvv_out <<"fs(" << abst << ") " 
            << ((abst==actual)? "==":"!=") 
            << " fs(" << actual << ")\n";
    return abst - actual;
}

int AbstFS::compare_obj(AbstFS& fs, ino_t ino, int &exists)
{
#ifdef DEBUG_ABST_FS
    actual_to_abst_t::iterator it;
    for(it = actual_to_abst_map.begin(); it != actual_to_abst_map.end(); it++) {
        cout << (*it).first << " " 
             << (*it).second << endl;
    }
    for(it = fs.actual_to_abst_map.begin(); it != fs.actual_to_abst_map.end(); it++) {
        cout << (*it).first << " " 
             << (*it).second << endl;
    }
#endif
    int result = 0;

    if(actual_to_abst_map.find(ino) == actual_to_abst_map.end()
       || fs.actual_to_abst_map.find(ino) == fs.actual_to_abst_map.end()) {
        exists = 0;
        cout << "WARN: couldn't find " << ino 
             << "in one of the file systems! " << endl;
        return 0;
    }
    exists = 1;
	
    int actual_id = actual_to_abst_map[ino];
    int stable_id = fs.actual_to_abst_map[ino];
    result = compare_obj(_objs[actual_id], fs._objs[stable_id]);
    if(!result)
        cout << "objs equal" << endl;
    return result;
}


int AbstFS::compare_obj(AbstFS& fs, const char* file, int &exists)
{
    int result = 0, ret = 0;
    namei_t nd = namei_init, nd_fs = namei_init;

    exists = 0;
    ret = walk_path(file, nd);
    if(ret < 0) {
        cout << "WARN: coudn't find " << file
             << "in one of the file systems! " << endl;
        return 0;
    }

    ret = fs.walk_path(file, nd_fs);
    if(ret < 0) {
        cout << "WARN: coudn't find " << file
             << "in one of the file systems! " << endl;
        return 0;
    }

    exists = 1;
    result = compare_obj(nd.obj, nd_fs.obj);
    if(!result)
        cout << "objs equal" << endl;
    return result;
}

int AbstFS::compare_obj(meta *o1, meta *o2)
{
    char actual_label[MAX_LABEL];
    char stable_label[MAX_LABEL];
    canon_obj(o1, actual_label, sizeof(actual_label));
    canon_obj(o2, stable_label, sizeof(stable_label));
    return memcmp(actual_label, stable_label, MAX_LABEL);
}

// this function is not used now
void AbstFS::condense_ids(void)
{
    assert(0);
    id_map_t id_map; /* obj id -> mapped id */
    obj_map_t::iterator it;
    unsigned i;
    for(it = _objs.begin(), i = 0; it != _objs.end(); it++, i++)
        id_map[(*it).first] = i;
    fix_ids(id_map);
}

void AbstFS::fix_ids(id_map_t& id_map)
{
    internal_check();

    obj_map_t tmp;
    tmp.swap(_objs);
    obj_map_t::iterator it;

    for(it = tmp.begin(); it != tmp.end(); it++) {
        int oldid = (*it).first;
        int newid = id_map[oldid];
        meta *o = (*it).second;
        dir *d;
        o->id = newid;
        _objs[newid] = o;
		
        // update dir entries
        d = CAST(dir*, o);
        if(d) {
            entry_map_t::iterator eit;
            for(eit = d->entries.begin(); eit != d->entries.end(); eit++)
                (*eit).second = id_map[(*eit).second];
        }
    }
    internal_check();
}

/* remap ids based on DFS traversal (pre order). i.e., remap
   object ids so that if an object's id is n, it's the nth object
   we see in DFS traversal where directory entries are actual_to_abst_map in
   strcmp order */
void AbstFS::dfs_remap_ids(void)
{
    id_map_t id_map;

    dfs_order(id_map, true);

#ifdef DEBUG_ABSTFS
    id_map_t::iterator it;
    for (it = id_map.begin(); it != id_map.end(); it++) 
        cout << (*it).first << "->" << (*it).second << endl;
#endif

    fix_ids(id_map);

    // reset _id
    _id = _objs.size();

#if 0
    // why do we need this?
    // reset _name_id
    // cout << "resetting nameid to " << _name_id << endl;
    _name_id = m.name_id + 1;
#endif

    // FIXME  hiden state.  do we still need it????
    // change actual_to_abst_map id
    actual_to_abst_t::iterator it2;
    for(it2 = actual_to_abst_map.begin();
        it2 != actual_to_abst_map.end(); it2++) {
        // actual_to_abst_map[(*it2).first] = m.id_map[(*it2).second];
        (*it2).second = id_map[(*it2).second];
    }

    internal_check();
}

struct delayed_removal {
    AbstFS::meta* parent;
    string entry_name;
};
void AbstFS::remove_ignored(void)
{
    class r: public visitor {
        AbstFS *fs;
        char path_tmp[MAX_PATH];
    public:
        vector<struct delayed_removal> removals;
        r(AbstFS *f): fs(f) {}
        visit_status_t post_visit(meta* parent, const char* name,
                                  meta* obj, int depth) {
            fs->find_back_path(obj, path_tmp, sizeof(path_tmp));
            // this is so ugly
            if(fs->_driver && fs->_driver->_actual_fs
               && fs->_driver->_actual_fs->ignore(path_tmp)) {
                struct delayed_removal dr;
                dr.parent = parent;
                dr.entry_name = name;
                removals.push_back(dr);
            }
        };
    };

    r r(this);
    dfs(r);
    vector<struct delayed_removal>::iterator it;
    for_all_iter(it, r.removals) 
        unlink_obj((*it).parent, (*it).entry_name.c_str());
}

// marshal the actual file system into an abstract one.  it
// generates a mapping (abst_to_actual_map) between the inode
// number of a file or dir in the actual file system, to the
// corresponding file or dir in the abstract file system
void AbstFS::abstract (const char* mnt)
{
    clear(); // clear before abstract

    _mnt = mnt;
    try {
        abstract_one (NULL, mnt, actual_to_abst_map);
    } catch (InvalidFS e) {
        cout << e.cause << endl;
        error("invalidfs", e.cause.c_str());
    }

    internal_check();

    dfs_remap_ids();

    // reset name id
    class mapper: public visitor {
    public:
        int name_id;
    public:
        mapper() {name_id = 0;}
        visit_status_t pre_visit(meta* parent, const char* name,
                                 meta* obj, int depth) {
            int nid = name_to_id(name);
            if(name_id < nid) name_id = nid;
            return VISIT_CONT;
        };
    };

    // remap ids based on DFS traversal
    mapper m;
    dfs(m);
    _name_id = m.name_id + 1;
}

void AbstFS::abstract_one (const char* paren_path,
			   const char* path, 
			   actual_to_abst_t &actual_to_abst_map)
{
    int ret=-1;
    struct stat st;
    char path_tmp[MAX_PATH] = {0};

    // if there is an actual fs attached to it, and actual fs says
    // ignore this path name, return
    if(_driver && _driver->_actual_fs->ignore(path))
        return;

    ret = ::lstat(path, &st);
    CHECK_RETURN(ret);

    vv_out << "abstracting " << path 
           << "[ino=" << st.st_ino << "]"
           << " size = " << st.st_size << "\n";

    if(paren_path == NULL) {
        // root
        actual_to_abst_map[st.st_ino] = 0;
        abstract_dir(path, actual_to_abst_map);
        return;
    }

    if(actual_to_abst_map.find(st.st_ino) != actual_to_abst_map.end()) {
        int old = actual_to_abst_map[st.st_ino];
        // already visited.  we can just link the object
        // pointer here.  now leave it this way
        find_back_path(_objs[old], path_tmp, 
                       sizeof(path_tmp));
        this->link(path_tmp, path);
        return;
    }

    // new file system object
    switch (st.st_mode & S_IFMT) {
    case S_IFDIR:
        this->mkdir (path, st.st_mode&0777);
        break;
    case S_IFLNK:
        ret = ::readlink(path, path_tmp, sizeof(path_tmp));
        CHECK_RETURN(ret);
        this->symlink(path_tmp, path);
        break;
    case S_IFREG:
        this->creat(path, st.st_mode&0777);
        this->abstract_data(path);
        break;
    default:
        assert(0);
    }
	
    // mark it visited
    meta *o;
    this->stat(path, o);
    actual_to_abst_map[st.st_ino] = o->id;
	
    if (S_ISDIR(st.st_mode)) {
        abstract_dir(path, actual_to_abst_map);
        return;
    }

    return;
}

void AbstFS::abstract_dir(const char* path,
			  actual_to_abst_t &actual_to_abst_map)
{
    // visit children
    DIR *d = opendir(path);
    if(!d) perror(path);
    assert(d);
    struct dirent *ent;
    while ((ent=readdir(d))) {
        // can't use static buffer here. this piece of
        // code is re-entrant
        if (!strcmp(ent->d_name, ".")
            || !strcmp(ent->d_name, "..")
#if 0
            // need to skip lost+found too
            || !strcmp(ent->d_name, "lost+found")
#endif
            )
            continue;
        char *nextpath = new char[MAX_PATH];
        assert(nextpath);
        strcpy(nextpath, path);
        strcat(nextpath, "/");
        strcat(nextpath, ent->d_name);
        abstract_one(path, nextpath, actual_to_abst_map);
        delete [] nextpath;
    }
    closedir(d);
    return;
}

void AbstFS::abstract_data(const char* path)
{
    int fd, ret, n, size;
    off_t off;
    static char buf[1024*16];

    fd = open(path, O_RDONLY);
    CHECK_RETURN(fd);

    off = 0;
    size = sizeof buf;
    while((n=::pread(fd, buf, size, off)) > 0) {
        ret = this->pwrite(path, buf, n, off);
        assert(ret == n);
        off += n;
		
        // FIXME: data after this offset must be sparse.  don't
        // want to iterat thru all those zeroes.  however, there
        // is no syscall that tells us this file has holes.
        // cheat...
        if(off > 33000 && off < 0x40000000)
            off = 0x3ffff000;
    }
    CHECK_RETURN(n);
	
    ret = close(fd);
    CHECK_RETURN(fd);
}

void AbstFS::check_objects(void)
{
    obj_map_t::iterator it;
    for(it = _objs.begin(); it != _objs.end(); it++) {
        meta *o = (*it).second;
        xi_assert(o->id == (*it).first, 
                       "object id(%d) and its position(%d) "\
                       "in _objs map don't match!", o->id, (*it).first);
        xi_assert(o->id >= 0, "invalid object id!");
        if(it != _objs.begin())
            xi_assert(o->id != 0, "root appears more than once!");

        switch(o->type) {
        case ABST_FILE:
            xi_assert(dynamic_cast<file*>(o), 
                           "dynamic_cast failed. memory corrupted!");
            break;
        case ABST_DIR:
            xi_assert(dynamic_cast<dir*>(o), 
                           "dynamic_cast failed. memory corrupted!");
            break;
        case ABST_SYM_LINK:
            xi_assert(dynamic_cast<sym_link*>(o), 
                           "dynamic_cast failed. memory corrupted!");
            break;
        default:
            xi_panic("invalid object type");
        }
    }
}

void AbstFS::internal_check(void)
{
    if(detect_circle())
        xi_panic("circle detected!");
    check_objects();
}

AbstFS::obj_map_t *AbstFS::get_objs(void)
{
    return &_objs;
}

#ifdef DEBUG_ABSTFS
int main(void)
{
    AbstFS *fs = new AbstFS;
    fs->print(cout);
    fs->mkdir("/1", 0777);
    fs->print(cout);
    fs->creat("/1/2", 0777);
    fs->print(cout);
    fs->link("/1/2", "/1/3");
    fs->print(cout);
    fs->link("/1/2", "/4");
    fs->print(cout);
    fs->symlink("/1/5", "/1/8");
    fs->print(cout);

    AbstFS::namei_t nd = namei_init;
    fs->walk_path("/4", nd);
    fs->find_back_path(nd.obj, path_tmp, sizeof(path_tmp));
    cout << path_tmp << endl;

    fs->unlink("/1/2");
    fs->print(cout);
    fs->unlink("/4");
    fs->print(cout);
    fs->unlink("/1/3");
    fs->print(cout);
#if 0
    fs->rmdir("/1");
    fs->print(cout);
#endif
    Archive ar;
    AbstFS fs2;

    fs->condense_ids();

    cout << "==========================================\n";
    fs->marshal(ar);
    fs->print(cout);
    cout << "sig = " << fs->get_sig_no_name() << endl;

    fs2.unmarshal(ar);
    fs2.print(cout);
    cout << "sig = " << fs2.get_sig_no_name() << endl;
    cout << "==========================================\n";

    Archive ar2;
    fs2.marshal(ar2);
    fs->clear();
    fs->unmarshal(ar2);

    cout << "==========================================\n";
    fs->print(cout);
    cout << "sig = " << fs->get_sig_no_name() << endl;

    fs2.print(cout);
    cout << "sig = " << fs2.get_sig_no_name() << endl;
    cout << "==========================================\n";

    delete fs;

    abst_fs.mount("/home/junfeng/tmp", 0, NULL);
    bi_creat("/home/junfeng/tmp/test", 0777);
    abst_fs.print(cout);
    bi_mkdir("/home/junfeng/tmp/hehe", 0777);
    abst_fs.print(cout);
    bi_link("/home/junfeng/tmp/test", "/home/junfeng/tmp/hehe/test");
    abst_fs.print(cout);
    bi_symlink("/home/junfeng/tmp/test", "/home/junfeng/tmp/hehe/test2");
    abst_fs.print(cout);
    bi_rename("/home/junfeng/tmp/test", "/home/junfeng/tmp/hehe/test3");
    abst_fs.print(cout);

    AbstFS fs3;
    fs3.mount("/home/junfeng/tmp", 0, NULL);
    fs3.abstract("/home/junfeng/tmp");
    fs3.print(cout);
    fs3.dfs_remap_ids();
    fs3.print(cout);
    return 0;
}
#endif
