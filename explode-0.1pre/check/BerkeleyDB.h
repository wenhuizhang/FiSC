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


#ifndef __BERKELEY_DB_H
#define __BERKELEY_DB_H

#include "driver/TestDriver.h"
#include "Component.h"
#include <string>
#include <db.h>
#include <vector>
#include "bdb-options.h"

class BerkeleyDB:public Component
{
private:
    DB *_db;
    DB_ENV *_env;
    std::string _file;
    int *_key, *_data;

    void set_string(int *str, int id, int len);
	
public:
    BerkeleyDB(const char* path, const char *db_file, HostLib* ekm): 
        Component(path, ekm) {
        _file = db_file;
        _key = (int *) malloc(get_option(berkeleydb, datalen));
        _data = (int *) malloc(get_option(berkeleydb, datalen));
        _in_kern = false;
    }
    ~BerkeleyDB() {
        free(_key);
        free(_data);
    }
    virtual void init(void);
    virtual void mount(void);
    virtual void umount(void);
    // mount is recover, in this case
    // virtual void umount(void);

    char *get_key(int id);
    char *get_data(int id);
    DB *get_db() { return _db; }
    DB_ENV *get_env() { return _env; }
    void env_dir_create();
    void env_open();
    void db_open();
};

#endif
