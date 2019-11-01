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


#include <errno.h>

#include "crash.h"
#include "storage/Fs.h"
#include "BerkeleyDB.h"
#include "bdb-options.h"

using namespace std;

class Test: public TestDriver
{
private:
    HostLib *_ekm;
    BerkeleyDB *_db;
    vector<int> _committed_records;
    int check_helper(vector<int>& committed, string& err_msg);

public:
    Test(HostLib *ekm, BerkeleyDB *db):_ekm(ekm), TestDriver(true), _db(db) {}
    virtual void mutate(void);
    int check(enum commit_epoch_t ct, int id);
};

void Test::mutate(void)
{
    int ret, id;

    DB *db = _db->get_db();
    DB_ENV *env = _db->get_env();
    DBT key, data;
    DB_TXN* tid;
    bzero(&key, sizeof key);
    bzero(&data, sizeof data);

    int keylen = get_option(berkeleydb, keylen);
    int datalen = get_option(berkeleydb, datalen);

    //key.data = _db->get_data()
    key.size = keylen;
    //data.data = datadata = (int *) malloc(datalen);
    data.size = datalen;

    int puts = get_option(berkeleydb, puts);
    int commits = get_option(berkeleydb, commits);

    forget_old_crashes();
    for(int i = 0; i < commits; i++) {
        //printf("Begin Transaction\n");
        if((ret = env->txn_begin(env, NULL, &tid, 0)) != 0) {
            env->err(env, ret, "DB_ENV->txn_begin");
            exit (1);
        }
        //printf("Began transaction\n");

        for(int j = 0; j < puts; j++) {
            id = i * puts + j;
            key.data = _db->get_key(id);
            data.data = _db->get_data(id);

            //printf("Putting\n");
            assert(!db->put(db, tid, &key, &data, 0));
            //printf("Done Putting\n");
        }

        //printf("Committing Transaction\n");
        begin_commit(id);
        //_ekm->ekm_app_record(BERKELEY_DB_COMMIT, sizeof id, (char *) &id);
        if((ret = tid->commit(tid, 0)) != 0) {
            env->err(env, ret, "DB_TXN->commit");
            exit (1);
        }
        //printf("Committed Transaction\n");
        //sync();

        //record_commit((char *) &id, sizeof id); 
        end_commit(id);
        _committed_records.push_back(id);
    }
}

int Test::check(enum commit_epoch_t ct, int id)
{
    vector<int> new_committed;
    string err_msg;

    switch(ct) {
    case before_commit:
    case after_commit:
        if(check_helper(_committed_records, err_msg) != 0)
            error("berkeleydb", err_msg.c_str());
        break;
    case in_commit:
        new_committed = _committed_records;
        new_committed.push_back(id);

        if(check_helper(_committed_records, err_msg) != 0
           && check_helper(new_committed, err_msg) != 0)
            error("berkeleydb", err_msg.c_str());
        break;
    }
    return CHECK_SUCCEEDED;
}

int Test::check_helper(vector<int>& committed, string& err_msg)
{
    DB *db;
    DBC *dbc;
    DBT key, data;  
    int ret, n;
    //int *keydata, *datadata;
    char *datadata;
    int err = 1;

    bzero(&key, sizeof key);
    bzero(&data, sizeof data);

    int keylen = get_option(berkeleydb, keylen);
    int datalen = get_option(berkeleydb, datalen);

    key.size = keylen;

    db = _db->get_db();

    for(int i = 0; i < (int)committed.size(); i++) {
        //for(int j = 0; j < keylen/4; j++)
        //		keydata[j] = i;
        //	for(int j = 0; j < datalen/4; j++)
        //		datadata[j] = i;
        key.data = _db->get_key(i);
        datadata = _db->get_data(i);


        if((ret = db->get(db, NULL, &key, &data, 0)) != 0) {
            db->err(db, ret, "Error: ");
            err_msg = "berkeleydb get key missing";
            goto ret;
        }

        if((int)data.size != datalen) {
            err_msg = "berkeleydb get length diff";
            goto ret;
        }


        if(memcmp(data.data, datadata, datalen)) {
            err_msg = "berkeleydb get data diff";
            goto ret;
        }
    }

    if((ret = db->cursor(db, NULL, &dbc, 0)) != 0) {
    	err_msg = "berkeleydb couldn't get cursor";
        goto ret;
    }

    while((ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0) {
        if((int)data.size != datalen) {
            err_msg = "berkeleydb get length diff";
            goto close_cursor;
        }

        int id = *(int *) data.data;
        datadata = _db->get_data(id);

        if(find(committed.begin(), committed.end(), id)
           == committed.end()) {
            printf("ID %d\n", id);
            err_msg = "new berkeleydb record appeared";
            goto close_cursor;
        }

        // yes we already checked above, but the first 4 bytes might be the
        // between a regular item and a corrupted new one
        if(memcmp(data.data, datadata, datalen)) {
            err_msg = "berkeleydb get data diff";
            goto close_cursor;
        }
    }
	
    err = 0;
 close_cursor:
    assert(!dbc->c_close(dbc));

 ret:
    return err;
}


void assemble(Component *&test, TestDriver *&driver)
{
    test = build_typical_fs_stack("/mnt/sbd0");

    BerkeleyDB *berkeley_db = new BerkeleyDB("/mnt/sbd0/txnapp",
                                             "/mnt/sbd0/txnapp/db", 
                                             test->ekm());
    assert(berkeley_db);
    berkeley_db->plug_child(test);

    test = berkeley_db;
	
    driver = new Test(test->ekm(), berkeley_db);
    assert(driver);
}
