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


#include <algorithm>
#include "Component.h"
#include "ekm_lib.h"

using namespace std;

Component::Component(const char* path, HostLib* ekm)
{
    _path.clear(); 
    _path = path; 
    _ekm = ekm; 
    _in_kern = true;;
    _size = 0;
}

// FIXME: crude hack. only return the last pid read from ps
int Component::get_pid(const char* name)
{
    int pid = -1;
    FILE *f;
    size_t n;
    static char cmd[4096];
    static char line[4096];

    sprintf(cmd, "ps -A | grep %s", name);
    f = popen(cmd, "r");	
    xi_assert(f, "can't open pipe\n");
    /*while(getline(&line, &n, f) != -1) { */
    while(fgets(line, sizeof line, f) != NULL) {
        // output from ps are in the format
        // 1840 ?        00:00:00 <thread name>
        if(pid != -1)
            xi_warn("%s not unique!  prev pid is %d", name, pid);

        sscanf(line, "%5d", &pid);
        vvv_out << line << name << " pid is " << pid << endl;
    }
    pclose(f);
    return pid;
}

#if 0
void Component::traverse(struct visitor *v)
{
    traverse_(this, v);
}

bool Component::traverse_(Component *c, struct visitor *v)
{
    bool cont = v->pre_visit(c);
    
    if(!cont)
        return cont;

    component_vec_t::iterator it;

    for_all_iter(it, c->children) {
        cont = traverse_((*it), v);
        if(!cont)
            return cont;
    }

    v->post_visit(c);
}
#endif

// can use a macro to prettify those functions
void Component::init_all(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it) {
        (*it)->init();
        (*it)->mount();
    }
}

void Component::mount_all(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it)
        (*it)->mount();
}

void Component::umount_all(void)
{
    iterator it;
    for(it=begin(); it!=end(); ++it)
        (*it)->umount();
}

void Component::recover_all(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it) {
        (*it)->recover();
        (*it)->mount();
    }
}

void Component::threads_all(threads_t& th) 
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it)
        (*it)->threads(th);
}

void Component::mount_user(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it)
        if((*it)->in_user())
            (*it)->mount();
}

void Component::umount_user(void)
{
    iterator it;
    for(it=begin(); it!=end(); ++it)
        if((*it)->in_user())
            (*it)->umount();
}

void Component::recover_user(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it)
        if((*it)->in_user()) {
            (*it)->recover();
            (*it)->mount();
        }
}

void Component::mount_kern(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it)
        if((*it)->in_kern())
            (*it)->mount();
}

void Component::umount_kern(void)
{
    iterator it;
    for(it=begin(); it!=end(); ++it)
        if((*it)->in_kern())
            (*it)->umount();
}

void Component::recover_kern(void)
{
    post_iterator it;
    for(it=pbegin(); it!=pend(); ++it)
        if((*it)->in_kern()) {
            (*it)->recover();
            (*it)->mount();
        }
}
