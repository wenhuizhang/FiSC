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


#include "BerkeleyDB.h"

void BerkeleyDB::set_string(int *str, int id, int len)
{
    char *cstr = (char *) str;
    char *cid = (char *) &id;

    for(int i = 0; i < len / sizeof(int); i++)
        str[i] = id;
    for(int i = 0; i < len % sizeof(int); i++) {
        // YJF: shouldn't the offset be multiplied by 4 ?????
        cstr[len - (len % sizeof(int)) + i] = cid[i];
    }
}

char *BerkeleyDB::get_key(int id)
{
    int keylen = get_option(berkeleydb, keylen);
    assert(_key);
    set_string(_key, id, keylen);
    return (char *)_key;
}

char *BerkeleyDB::get_data(int id)
{
    int datalen = get_option(berkeleydb, datalen);
    assert(_data);
    set_string(_data, id, datalen);
    return (char *)_data;
}

void BerkeleyDB::init(void)
{
    // copy test repository
    //int pos = _path.rfind('/');
    //_name = _path.substr(pos+1);
    //nsystemf(_ekm, "cp -r %s %s", _name.c_str(), path());
  
    _ekm->systemf("rm -rf %s", _path.c_str());

    // TODO later construct _file here manually

    //string dir = "/tmp/work", file;
    //checkout(dir, file);

    // create environment
    env_dir_create();

    // open environment
    env_open();  	

    // SHOULD BE HERE
    sync();

    // open database
    db_open();

    umount();

    // make sure everything is on disk before we check cvs commit
    sync();
}

void BerkeleyDB::mount(void)
{
    env_open();  	
    db_open();
}

void BerkeleyDB::umount(void)
{
    int ret = -1, retries;

    retries = 10;
    while((ret=_db->close(_db, 0)) != 0) {
        fprintf(stderr, "close db failed");
        if(--retries == 0)
            xi_panic("can't close db");
    }
    retries = 10;
    while((ret=_env->close(_env, 0)) != 0) {
        fprintf(stderr, "close env failed");
        if(ret == DB_RUNRECOVERY) // needs to run recovery
            break;
        if(--retries == 0)
            xi_panic("can't close env");
    }
}

void BerkeleyDB::env_dir_create()
{
    struct stat sb;

    /*
     * If the directory exists, we're done. We do not further check
     * the type of the file, DB will fail appropriately if it's the
     * wrong type. */
    if (stat(path(), &sb) == 0)
        return;

    /* Create the directory, read/write/access owner only. */
    if (mkdir(path(), S_IRWXU) != 0) {
        fprintf(stderr, "txnapp: mkdir: %s: %s\n", path(), strerror(errno));
        exit (1);
    }
}


void BerkeleyDB::env_open()
{
    DB_ENV *dbenv;
    int ret;

    /* Create the environment handle. */
    if ((ret = db_env_create(&dbenv, 0)) != 0) {
        fprintf(stderr, "txnapp: db_env_create: %s\n", db_strerror(ret));
        exit (1);
    }

    /* Set up error handling. */
    dbenv->set_errpfx(dbenv, "txnapp");
    dbenv->set_errfile(dbenv, stderr);

    /*
     * Open a transactional environment: 
     * create if it doesn't exist 
     * free-threaded handle 
     * run recovery 
     * read/write owner only */
    if ((ret = dbenv->open(dbenv, path(), 
                           DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | 
                           DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER,
                           S_IRUSR | S_IWUSR)) != 0) {
        (void)dbenv->close(dbenv, 0);
        error("berkeleydb", "dbenv->open %s: %s\n", path(), db_strerror(ret));	
    }

    _env = dbenv;
}

void BerkeleyDB::db_open()
{
    if(db_create(&_db, _env, 0) < 0) {
    	fprintf(stderr, "Couldn't create database handle\n");
    	exit(-1);
    } 

    assert(!_db->set_pagesize(_db, 512));

    if(_db->open(_db, NULL /* txid */, _file.c_str(), NULL /* database */, DB_BTREE,
                 DB_CREATE | DB_AUTO_COMMIT, 0)) {
    	fprintf(stderr, "Couldn't open database file\n");
    	exit(-1);
    }
}

