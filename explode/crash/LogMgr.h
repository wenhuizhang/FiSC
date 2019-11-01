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


#ifndef __LOG_MGR_H
#define __LOG_MGR_H

#include <list>
#include "hash_map_compat.h"

#include "ekm_log_types.h"
#include "RawLog.h"

class Log;
class RawLog;
class RecParser;
class Record;

class LogMgr {
public:

    LogMgr(void);
    virtual ~LogMgr(void);

    rec_cat_t new_category(const char* name, RecParser* p = NULL);
    rec_type_t new_type(rec_cat_t cat, const char* name);

    void reg_existing_category(rec_cat_t cat, const char* name, RecParser* p);
    void reg_existing_type(rec_type_t type, const char* name);

    Log* parse(RawLog* raw_log);

    const char* category_name(rec_cat_t cat);
    const char* name(rec_type_t type);
    const char* name(Record *rec);

    static LogMgr* the(void);

protected:

    class Category;
    typedef std::list<Category*> categories_t;
    typedef Sgi::hash_map<rec_cat_t, Category*>  category_registry_t;


    Sgi::hash_map<rec_type_t, std::string> _type_names;
    categories_t _categories;
    category_registry_t _reg;
    rec_cat_t _cur_cat;
    
    static LogMgr* _the_one;
};

#endif // #ifndef __LOG_MGR_H
