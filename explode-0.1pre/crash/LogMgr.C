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


#include <assert.h>

#include "util.h"
#include "unmarshal.h"
#include "ekm_log_types.h"
#include "RawLog.h"
#include "Log.h"

using namespace std;

class LogMgr::Category {
    friend class LogMgr;
    
    Category(const char* name, RecParser* p)
        :_name(name), _parse(p)
    { _cur_type = 0; }

    ~Category(void)
    { delete _parse; }

    string _name;
    RecParser *_parse;
    rec_type_t _cur_type;
};

Log* LogMgr::parse(RawLog* raw_log)
{
    rec_type_t type;
    rec_cat_t cat;
    list<Record*> result;

    Log* l = new Log;

    // start
    categories_t::iterator it;
    for_all_iter(it, _categories)
        (*it)->_parse->start(l);

    // parse
    const char* off = raw_log->log();
    while(off < raw_log->log() + raw_log->size()) {

        unmarshal(off, type);
        cat = EKM_CAT(type);
        
        if(!exists(cat, _reg))
            xi_panic("unhandled log type 0x%x", type);

        //vn_out(4) << "parsing " << _type_names[type] << "\n";
        (*_reg[cat]->_parse)(type, off, l);
    }

    xi_assert(off == raw_log->log() + raw_log->size(),
              "log corrupted");

    // finish
    for_all_iter(it, _categories)
        (*it)->_parse->finish(l);

    if(output_vvv) {
        static int count = 0;
        Log::iterator i;
        cout << "==========BEGIN LOG " <<count++<< "=========\n";
        for_all_iter(i, *l)
            cout << *(*i) << "\n";
        cout << "==========  END LOG =========\n";
    }

    return l;
}

rec_cat_t LogMgr::new_category(const char* name, RecParser* p)
{
    ++ _cur_cat;
    if(_cur_cat >= EKM_USR_CAT_END)
        xi_panic("can't create new category: too many categories!");
    
    assert(!exists(_cur_cat, _reg));

    // by default, all user records are parsed by DefaultParser<AppRec>
    if(p == NULL)
        p = new DefaultParser<AppRec>;

    p->interest(_cur_cat);
    _categories.push_back(_reg[_cur_cat] = new Category(name, p));
    return _cur_cat;
}

rec_type_t LogMgr::new_type(rec_cat_t cat, const char* name)
{
    assert(exists(cat, _reg));

    ++ _reg[cat]->_cur_type;
    if(_reg[cat]->_cur_type >= EKM_MAX_MINOR)
        xi_panic("can't create new type for %d: too many types!", cat);

    rec_type_t t = EKM_MAKE_TYPE(cat, _reg[cat]->_cur_type);
    _type_names[t] = name;
    return t;
}

void LogMgr::reg_existing_category(rec_cat_t cat, 
                                   const char* name, RecParser* p)
{
    assert(!exists(cat, _reg));
    p->interest(_cur_cat);
    _categories.push_back(_reg[cat] = new Category(name, p));
}

void LogMgr::reg_existing_type(rec_type_t type, const char* name)
{
    _type_names[type] = name;
}

const char* LogMgr::category_name(rec_cat_t cat)
{
    assert(exists(cat, _reg));
    return _reg[cat]->_name.c_str();
}

const char* LogMgr::name(rec_type_t type)
{
    assert(exists(type, _type_names));
    return _type_names[type].c_str();
}

const char* LogMgr::name(Record *rec)
{
    return name(rec->type());
}

LogMgr::LogMgr(void)
{
    _cur_cat = EKM_USR_CAT_BEGIN;

}

LogMgr::~LogMgr(void)
{
    categories_t::iterator it;
    for_all_iter(it, _categories)
        delete *it;
}

#ifdef __linux
#    include "LinuxLog.h"
#endif

LogMgr* LogMgr::_the_one = NULL;
LogMgr* LogMgr::the(void)
{
    if(_the_one == NULL) {
        _the_one = new LogMgr;

        std::cout<<"created log mgr"<<std::endl;

        // register existing categories and types
        SectorRecParser::reg();
        BlockRecParser::reg();
        SyscallRecParser::reg();

#ifdef __linux
        LinuxBlockRecParser::reg();
#endif
    }

    return _the_one;
}
