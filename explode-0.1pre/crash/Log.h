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



/* TODO: printing routines for each record class */

#ifndef __LOG_H
#define __LOG_H

#include <list>
#include "hash_map_compat.h"
#include <boost/shared_ptr.hpp>

#include "mcl.h"
#include "ekm_log_types.h"
#include "sector_loc.h"
#include "unmarshal.h"
#include "LogMgr.h"

class Record {
    
protected:
    rec_type_t _type;  // type of this record

    int _lsn;   // in the raw ekm log, this is the unique,
                // consecutive, monotonic increasing sequence
                // number.  However, after we convert a raw log
                // into a format that is easier to replay, this is
                // LSN may no longer be unique and consecutive.
                // For example, we break one buffer record into a
                // bunch of sector records w/o the same LSN.

    int _pid;   // id of the kernel thread who wrote this record

    int _magic; // this must match EKM_LOG_MAGIC

    const char* _off; // points to starting offset of
                      // corresponding raw record in the raw ekm
                      // log
public:

    // this constructor moves @log while it unmarshals record fields
    Record(rec_type_t t, const char* &off);

    // creates an empty record
    Record(rec_type_t t);

    virtual ~Record() {}

    rec_cat_t category(void) const {return EKM_CAT(_type);}
    rec_type_t type(void) const {return _type;}
    int lsn(void) const {return _lsn;}
    const char* off(void) const {return _off;}

    void type(rec_type_t t) {_type = t;}
    void off(const char* off) {_off = off;}

    virtual std::ostream& print(std::ostream& o) const;

    bool is_buffer(void)
    { return category() == EKM_BLOCK_CAT; }
    bool is_sector(void)
    { return category() == EKM_SECTOR_CAT; }
    bool is_syscall(void)
    { return category() == EKM_SYSCALL_CAT; }
};

static inline std::ostream& 
operator<<(std::ostream& o, const Record& r)
{  return r.print(o); }

class BlockRec: public Record {
    
protected:
    sector_loc_t _loc; // (device, sector) pair

    int _flags; // UNUSED.  For blocks containing commit record,
                // EKM_FL_COMMIT must be set.  For JFS and XFS,
                // treat all log writes as commit records.

    int _size; // size of the buffer

    char *_data; // buffer data. maybe null

public:

    BlockRec(rec_type_t t, const char* &off, bool has_data=true);
    ~BlockRec(void);

    const sector_loc_t& loc(void) const {return _loc;} 
    int flags(void) const {return _flags;}
    int size(void) const {return _size;}
    char* data(void) const {return _data;}
    char* sector(int s) const {return _data + s * SECTOR_SIZE;}

    int device(void) const {return _loc.first;}
    int sector(void) const {return _loc.second;}
    int nsector() const {return _size / SECTOR_SIZE;}

    virtual std::ostream& print(std::ostream& o) const;
};



class SectorRec: public Record {

protected:
    
    sector_loc_t _loc;
    int _flags; // see comments on _flags field of class BlockRec
    signature_t _sig;

public:
    SectorRec(const BlockRec &buf, 
              rec_type_t type, int sector, const signature_t& sig);
    SectorRec(const BlockRec &buf, int sector);

    const sector_loc_t& loc(void) const {return _loc;}
    const signature_t& sig(void) const {return _sig;}
    int device(void) const {return _loc.first;}
    int sector(void) const {return _loc.second;}

    virtual std::ostream& print(std::ostream& o) const;
};



typedef Sgi::hash_map<int, int> fd_map_t;

class SyscallRec: public Record {

public:

    SyscallRec(rec_type_t t, const char* &off): Record(t, off) {}

    virtual void pair(SyscallRec *rec) = 0;
    virtual void replay(fd_map_t& open_fds) {};
    virtual bool is_enter(void) {return false;}
    bool is_exit(void) {return !is_enter();}
};



class AppRec: public Record {

protected:
    int _size;
    char *_data;

public:
    AppRec(rec_type_t t, const char* &off);
    AppRec(rec_type_t t);
    ~AppRec(void);
};


class Log: public std::list<Record*>
{
    typedef std::list<Record*> _Parent;

public:
    ~Log() {clear();}
    void clear(void);
};
typedef boost::shared_ptr<Log> LogPtr;


class RecParser {

public:

    void interest(rec_cat_t i)
    { _interest = i; }

    rec_cat_t interest(void) const 
    { return _interest; }

    virtual void start(Log* l) {}

    virtual void operator()(rec_type_t t, const char* &off, 
                            Log* l) = 0;

    virtual void finish(Log* l) {}

protected:
    rec_cat_t _interest;
};



template<class R>
class DefaultParser: public RecParser
{
public:
    virtual void operator()(rec_type_t t, const char* &off, 
                            Log* l) {
        Record *r = new R(t, off);
        vn_out(4) << "parsing " << *r << "\n"; 
        l->push_back(r);
    }
};



class BlockRecParser: public RecParser {
public:
    // TODO: break one buffer record into several sector records
    virtual void operator()(rec_type_t t, const char* &off, 
                            Log* l);

    static void reg(void);
};



class SectorRecParser: public RecParser
{
public:
    virtual void operator()(rec_type_t t, const char* &off, 
                            Log* l) {
        xi_panic("Shouldn't see Sector records in the ekm raw log.  "\
                 "These records are converted from other records "\
                 "(e.g. BlockRec).");
    }

    static void reg(void);
};



#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
// the code below is auto-generated, to unmarshall system call records
/* BEGIN SYSCALL */
/* 
 *   Everything between BEGIN SYSCALL and END SYSCALL 
 *   is automatically generated from ./scripts/gen_syscall_log.pl 
 */


class ExitChdirRec;
class EnterChdirRec: public SyscallRec {

public:
    ExitChdirRec* other_half;
     char*  filename;

    EnterChdirRec(const char* &off): SyscallRec(EKM_enter_chdir, off) {
        unmarshal(off, filename);
    }
    ~EnterChdirRec(void) {
        delete [] filename;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":filename="<<filename;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitChdirRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterChdirRec;
class ExitChdirRec: public SyscallRec {

public:
    EnterChdirRec* other_half;
     long  ret;

    ExitChdirRec(const char* &off): SyscallRec(EKM_exit_chdir, off) {
        unmarshal(off, ret);
    }
    ~ExitChdirRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterChdirRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterChdirRec* enter = (EnterChdirRec*) other_half;

        ExitChdirRec*   exit = (ExitChdirRec*)  this;
    ;
     long  replay_ret =  chdir(enter->filename);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    }

};


class ExitCloseRec;
class EnterCloseRec: public SyscallRec {

public:
    ExitCloseRec* other_half;
    unsigned int  fd;

    EnterCloseRec(const char* &off): SyscallRec(EKM_enter_close, off) {
        unmarshal(off, fd);
    }
    ~EnterCloseRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitCloseRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterCloseRec;
class ExitCloseRec: public SyscallRec {

public:
    EnterCloseRec* other_half;
     long  ret;

    ExitCloseRec(const char* &off): SyscallRec(EKM_exit_close, off) {
        unmarshal(off, ret);
    }
    ~ExitCloseRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterCloseRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterCloseRec* enter = (EnterCloseRec*) other_half;

        ExitCloseRec*   exit = (ExitCloseRec*)  this;
    ;
     long  replay_ret =  close(open_fds[enter->fd]);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
            if(replay_ret >= 0) open_fds.erase(ret);
;
    ;
    }

};


class ExitFdatasyncRec;
class EnterFdatasyncRec: public SyscallRec {

public:
    ExitFdatasyncRec* other_half;
    unsigned int  fd;

    EnterFdatasyncRec(const char* &off): SyscallRec(EKM_enter_fdatasync, off) {
        unmarshal(off, fd);
    }
    ~EnterFdatasyncRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitFdatasyncRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterFdatasyncRec;
class ExitFdatasyncRec: public SyscallRec {

public:
    EnterFdatasyncRec* other_half;
     long  ret;

    ExitFdatasyncRec(const char* &off): SyscallRec(EKM_exit_fdatasync, off) {
        unmarshal(off, ret);
    }
    ~ExitFdatasyncRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterFdatasyncRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterFdatasyncRec* enter = (EnterFdatasyncRec*) other_half;

        ExitFdatasyncRec*   exit = (ExitFdatasyncRec*)  this;
    ;
     long  replay_ret =  fdatasync(open_fds[enter->fd]);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    }

};


class ExitFsyncRec;
class EnterFsyncRec: public SyscallRec {

public:
    ExitFsyncRec* other_half;
    unsigned int  fd;

    EnterFsyncRec(const char* &off): SyscallRec(EKM_enter_fsync, off) {
        unmarshal(off, fd);
    }
    ~EnterFsyncRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitFsyncRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterFsyncRec;
class ExitFsyncRec: public SyscallRec {

public:
    EnterFsyncRec* other_half;
     long  ret;

    ExitFsyncRec(const char* &off): SyscallRec(EKM_exit_fsync, off) {
        unmarshal(off, ret);
    }
    ~ExitFsyncRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterFsyncRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterFsyncRec* enter = (EnterFsyncRec*) other_half;

        ExitFsyncRec*   exit = (ExitFsyncRec*)  this;
    ;
     long  replay_ret =  fsync(open_fds[enter->fd]);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    }

};


class ExitFtruncateRec;
class EnterFtruncateRec: public SyscallRec {

public:
    ExitFtruncateRec* other_half;
    unsigned int  fd;
    unsigned long  length;

    EnterFtruncateRec(const char* &off): SyscallRec(EKM_enter_ftruncate, off) {
        unmarshal(off, fd);
    unmarshal(off, length);
    }
    ~EnterFtruncateRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd<<":length="<<length;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitFtruncateRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterFtruncateRec;
class ExitFtruncateRec: public SyscallRec {

public:
    EnterFtruncateRec* other_half;
     long  ret;

    ExitFtruncateRec(const char* &off): SyscallRec(EKM_exit_ftruncate, off) {
        unmarshal(off, ret);
    }
    ~ExitFtruncateRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterFtruncateRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterFtruncateRec* enter = (EnterFtruncateRec*) other_half;

        ExitFtruncateRec*   exit = (ExitFtruncateRec*)  this;
    ;
    ;
     long  replay_ret =  ftruncate(open_fds[enter->fd], enter->length);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    }

};


class ExitLinkRec;
class EnterLinkRec: public SyscallRec {

public:
    ExitLinkRec* other_half;
     char * oldname;
     char * newname;

    EnterLinkRec(const char* &off): SyscallRec(EKM_enter_link, off) {
        unmarshal(off, oldname);
    unmarshal(off, newname);
    }
    ~EnterLinkRec(void) {
        delete [] oldname;
    delete [] newname;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":oldname="<<oldname<<":newname="<<newname;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitLinkRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterLinkRec;
class ExitLinkRec: public SyscallRec {

public:
    EnterLinkRec* other_half;
     long  ret;

    ExitLinkRec(const char* &off): SyscallRec(EKM_exit_link, off) {
        unmarshal(off, ret);
    }
    ~ExitLinkRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterLinkRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterLinkRec* enter = (EnterLinkRec*) other_half;

        ExitLinkRec*   exit = (ExitLinkRec*)  this;
    ;
    ;
     long  replay_ret =  link(enter->oldname, enter->newname);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    }

};


class ExitLlseekRec;
class EnterLlseekRec: public SyscallRec {

public:
    ExitLlseekRec* other_half;
    unsigned int  fd;
    unsigned long  offset_high;
    unsigned long  offset_low;
    unsigned int  origin;

    EnterLlseekRec(const char* &off): SyscallRec(EKM_enter_llseek, off) {
        unmarshal(off, fd);
    unmarshal(off, offset_high);
    unmarshal(off, offset_low);
    unmarshal(off, origin);
    }
    ~EnterLlseekRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd<<":offset_high="<<offset_high<<":offset_low="<<offset_low<<":origin="<<origin;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitLlseekRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterLlseekRec;
class ExitLlseekRec: public SyscallRec {

public:
    EnterLlseekRec* other_half;
     long  ret;
     loff_t *  result;

    ExitLlseekRec(const char* &off): SyscallRec(EKM_exit_llseek, off) {
        unmarshal(off, ret);
    unmarshal(off, result);
    }
    ~ExitLlseekRec(void) {
        delete result;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterLlseekRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        
        EnterLlseekRec* enter = (EnterLlseekRec*) other_half;
		off64_t off_tmp = ((off64_t)(enter->offset_high))<<32 | enter->offset_low;
		off64_t replay_ret = lseek64(enter->fd, off_tmp, enter->origin);
    if(replay_ret != *result) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);;
    }

};


class ExitLseekRec;
class EnterLseekRec: public SyscallRec {

public:
    ExitLseekRec* other_half;
    unsigned int  fd;
    off_t  offset;
    unsigned int  origin;

    EnterLseekRec(const char* &off): SyscallRec(EKM_enter_lseek, off) {
        unmarshal(off, fd);
    unmarshal(off, offset);
    unmarshal(off, origin);
    }
    ~EnterLseekRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd<<":offset="<<offset<<":origin="<<origin;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitLseekRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterLseekRec;
class ExitLseekRec: public SyscallRec {

public:
    EnterLseekRec* other_half;
     off_t  ret;

    ExitLseekRec(const char* &off): SyscallRec(EKM_exit_lseek, off) {
        unmarshal(off, ret);
    }
    ~ExitLseekRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterLseekRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterLseekRec* enter = (EnterLseekRec*) other_half;

        ExitLseekRec*   exit = (ExitLseekRec*)  this;
    ;
    ;
    ;
     off_t  replay_ret =  lseek(open_fds[enter->fd], enter->offset, enter->origin);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    ;
    }

};


class ExitMkdirRec;
class EnterMkdirRec: public SyscallRec {

public:
    ExitMkdirRec* other_half;
     char *  pathname;
    int  mode;

    EnterMkdirRec(const char* &off): SyscallRec(EKM_enter_mkdir, off) {
        unmarshal(off, pathname);
    unmarshal(off, mode);
    }
    ~EnterMkdirRec(void) {
        delete [] pathname;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":pathname="<<pathname<<":mode="<<mode;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitMkdirRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterMkdirRec;
class ExitMkdirRec: public SyscallRec {

public:
    EnterMkdirRec* other_half;
     long  ret;

    ExitMkdirRec(const char* &off): SyscallRec(EKM_exit_mkdir, off) {
        unmarshal(off, ret);
    }
    ~ExitMkdirRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterMkdirRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterMkdirRec* enter = (EnterMkdirRec*) other_half;

        ExitMkdirRec*   exit = (ExitMkdirRec*)  this;
    ;
    ;
     long  replay_ret =  mkdir(enter->pathname, enter->mode);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    }

};


class ExitOpenRec;
class EnterOpenRec: public SyscallRec {

public:
    ExitOpenRec* other_half;
     char * filename;
    int  flags;
    int  mode;

    EnterOpenRec(const char* &off): SyscallRec(EKM_enter_open, off) {
        unmarshal(off, filename);
    unmarshal(off, flags);
    unmarshal(off, mode);
    }
    ~EnterOpenRec(void) {
        delete [] filename;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":filename="<<filename<<":flags="<<flags<<":mode="<<mode;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitOpenRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterOpenRec;
class ExitOpenRec: public SyscallRec {

public:
    EnterOpenRec* other_half;
     long  ret;

    ExitOpenRec(const char* &off): SyscallRec(EKM_exit_open, off) {
        unmarshal(off, ret);
    }
    ~ExitOpenRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterOpenRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterOpenRec* enter = (EnterOpenRec*) other_half;

        ExitOpenRec*   exit = (ExitOpenRec*)  this;
    ;
    ;
    ;
     long  replay_ret =  open(enter->filename, enter->flags, enter->mode);
    ;
            if(replay_ret >= 0) open_fds[ret] = replay_ret;
;
    ;
    ;
    ;
    }

};


class ExitPwrite64Rec;
class EnterPwrite64Rec: public SyscallRec {

public:
    ExitPwrite64Rec* other_half;
    unsigned int  fd;
      char * buf;
    size_t  count;
    loff_t  pos;

    EnterPwrite64Rec(const char* &off): SyscallRec(EKM_enter_pwrite64, off) {
        unmarshal(off, fd);
    unmarshal_array(off, buf);
    unmarshal(off, count);
    unmarshal(off, pos);
    }
    ~EnterPwrite64Rec(void) {
        delete [] buf;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd<<":count="<<count<<":pos="<<pos;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitPwrite64Rec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterPwrite64Rec;
class ExitPwrite64Rec: public SyscallRec {

public:
    EnterPwrite64Rec* other_half;
     ssize_t  ret;

    ExitPwrite64Rec(const char* &off): SyscallRec(EKM_exit_pwrite64, off) {
        unmarshal(off, ret);
    }
    ~ExitPwrite64Rec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterPwrite64Rec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterPwrite64Rec* enter = (EnterPwrite64Rec*) other_half;

        ExitPwrite64Rec*   exit = (ExitPwrite64Rec*)  this;
    ;
    ;
    ;
    ;
     ssize_t  replay_ret =  pwrite64(open_fds[enter->fd], enter->buf, enter->count, enter->pos);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    ;
    ;
    }

};


class ExitReadRec;
class EnterReadRec: public SyscallRec {

public:
    ExitReadRec* other_half;
    unsigned int  fd;
    size_t  count;

    EnterReadRec(const char* &off): SyscallRec(EKM_enter_read, off) {
        unmarshal(off, fd);
    unmarshal(off, count);
    }
    ~EnterReadRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd<<":count="<<count;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitReadRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterReadRec;
class ExitReadRec: public SyscallRec {

public:
    EnterReadRec* other_half;
     ssize_t  ret;
       char * buf;

    ExitReadRec(const char* &off): SyscallRec(EKM_exit_read, off) {
        unmarshal(off, ret);
    unmarshal_array(off, buf);
    }
    ~ExitReadRec(void) {
        delete [] buf;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterReadRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterReadRec* enter = (EnterReadRec*) other_half;

        ExitReadRec*   exit = (ExitReadRec*)  this;
    ;
    char* buf = new char [enter->count];
    ;
     ssize_t  replay_ret =  read(open_fds[enter->fd], exit->buf, enter->count);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    delete [] buf;
    ;
    }

};


class ExitRenameRec;
class EnterRenameRec: public SyscallRec {

public:
    ExitRenameRec* other_half;
     char * oldname;
     char * newname;

    EnterRenameRec(const char* &off): SyscallRec(EKM_enter_rename, off) {
        unmarshal(off, oldname);
    unmarshal(off, newname);
    }
    ~EnterRenameRec(void) {
        delete [] oldname;
    delete [] newname;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":oldname="<<oldname<<":newname="<<newname;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitRenameRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterRenameRec;
class ExitRenameRec: public SyscallRec {

public:
    EnterRenameRec* other_half;
     long  ret;

    ExitRenameRec(const char* &off): SyscallRec(EKM_exit_rename, off) {
        unmarshal(off, ret);
    }
    ~ExitRenameRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterRenameRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterRenameRec* enter = (EnterRenameRec*) other_half;

        ExitRenameRec*   exit = (ExitRenameRec*)  this;
    ;
    ;
     long  replay_ret =  rename(enter->oldname, enter->newname);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    }

};


class ExitRmdirRec;
class EnterRmdirRec: public SyscallRec {

public:
    ExitRmdirRec* other_half;
     char *  pathname;

    EnterRmdirRec(const char* &off): SyscallRec(EKM_enter_rmdir, off) {
        unmarshal(off, pathname);
    }
    ~EnterRmdirRec(void) {
        delete [] pathname;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":pathname="<<pathname;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitRmdirRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterRmdirRec;
class ExitRmdirRec: public SyscallRec {

public:
    EnterRmdirRec* other_half;
     long  ret;

    ExitRmdirRec(const char* &off): SyscallRec(EKM_exit_rmdir, off) {
        unmarshal(off, ret);
    }
    ~ExitRmdirRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterRmdirRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterRmdirRec* enter = (EnterRmdirRec*) other_half;

        ExitRmdirRec*   exit = (ExitRmdirRec*)  this;
    ;
     long  replay_ret =  rmdir(enter->pathname);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    }

};


class ExitSymlinkRec;
class EnterSymlinkRec: public SyscallRec {

public:
    ExitSymlinkRec* other_half;
     char * oldname;
     char * newname;

    EnterSymlinkRec(const char* &off): SyscallRec(EKM_enter_symlink, off) {
        unmarshal(off, oldname);
    unmarshal(off, newname);
    }
    ~EnterSymlinkRec(void) {
        delete [] oldname;
    delete [] newname;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":oldname="<<oldname<<":newname="<<newname;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitSymlinkRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterSymlinkRec;
class ExitSymlinkRec: public SyscallRec {

public:
    EnterSymlinkRec* other_half;
     long  ret;

    ExitSymlinkRec(const char* &off): SyscallRec(EKM_exit_symlink, off) {
        unmarshal(off, ret);
    }
    ~ExitSymlinkRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterSymlinkRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterSymlinkRec* enter = (EnterSymlinkRec*) other_half;

        ExitSymlinkRec*   exit = (ExitSymlinkRec*)  this;
    ;
    ;
     long  replay_ret =  symlink(enter->oldname, enter->newname);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    }

};


class ExitSyncRec;
class EnterSyncRec: public SyscallRec {

public:
    ExitSyncRec* other_half;
    ;

    EnterSyncRec(const char* &off): SyscallRec(EKM_enter_sync, off) {
        ;
    }
    ~EnterSyncRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) ;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitSyncRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterSyncRec;
class ExitSyncRec: public SyscallRec {

public:
    EnterSyncRec* other_half;
     long  ret;

    ExitSyncRec(const char* &off): SyscallRec(EKM_exit_sync, off) {
        unmarshal(off, ret);
    }
    ~ExitSyncRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterSyncRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterSyncRec* enter = (EnterSyncRec*) other_half;

        ExitSyncRec*   exit = (ExitSyncRec*)  this;
    ;
     sync();
    ;
    ;
    ;
    }

};


class ExitTruncateRec;
class EnterTruncateRec: public SyscallRec {

public:
    ExitTruncateRec* other_half;
     char * filename;
    unsigned long  length;

    EnterTruncateRec(const char* &off): SyscallRec(EKM_enter_truncate, off) {
        unmarshal(off, filename);
    unmarshal(off, length);
    }
    ~EnterTruncateRec(void) {
        delete [] filename;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":filename="<<filename<<":length="<<length;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitTruncateRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterTruncateRec;
class ExitTruncateRec: public SyscallRec {

public:
    EnterTruncateRec* other_half;
     long  ret;

    ExitTruncateRec(const char* &off): SyscallRec(EKM_exit_truncate, off) {
        unmarshal(off, ret);
    }
    ~ExitTruncateRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterTruncateRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterTruncateRec* enter = (EnterTruncateRec*) other_half;

        ExitTruncateRec*   exit = (ExitTruncateRec*)  this;
    ;
    ;
     long  replay_ret =  truncate(enter->filename, enter->length);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    }

};


class ExitUnlinkRec;
class EnterUnlinkRec: public SyscallRec {

public:
    ExitUnlinkRec* other_half;
     char * pathname;

    EnterUnlinkRec(const char* &off): SyscallRec(EKM_enter_unlink, off) {
        unmarshal(off, pathname);
    }
    ~EnterUnlinkRec(void) {
        delete [] pathname;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":pathname="<<pathname;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitUnlinkRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterUnlinkRec;
class ExitUnlinkRec: public SyscallRec {

public:
    EnterUnlinkRec* other_half;
     long  ret;

    ExitUnlinkRec(const char* &off): SyscallRec(EKM_exit_unlink, off) {
        unmarshal(off, ret);
    }
    ~ExitUnlinkRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterUnlinkRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterUnlinkRec* enter = (EnterUnlinkRec*) other_half;

        ExitUnlinkRec*   exit = (ExitUnlinkRec*)  this;
    ;
     long  replay_ret =  unlink(enter->pathname);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    }

};


class ExitWriteRec;
class EnterWriteRec: public SyscallRec {

public:
    ExitWriteRec* other_half;
    unsigned int  fd;
      char * buf;
    size_t  count;

    EnterWriteRec(const char* &off): SyscallRec(EKM_enter_write, off) {
        unmarshal(off, fd);
    unmarshal_array(off, buf);
    unmarshal(off, count);
    }
    ~EnterWriteRec(void) {
        delete [] buf;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":fd="<<fd<<":count="<<count;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (ExitWriteRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return true;}

    virtual void replay(fd_map_t& open_fds) {
        ;
    }

};


class EnterWriteRec;
class ExitWriteRec: public SyscallRec {

public:
    EnterWriteRec* other_half;
     ssize_t  ret;

    ExitWriteRec(const char* &off): SyscallRec(EKM_exit_write, off) {
        unmarshal(off, ret);
    }
    ~ExitWriteRec(void) {
        ;
    }

    virtual std::ostream& print(std::ostream& o) const {
        return (Record::print(o)) <<":ret="<<ret;
    }
    
    virtual void pair(SyscallRec* sr) {
        other_half = (EnterWriteRec*)(sr);
    }

    virtual bool is_enter(void) const
    { return false;}

    virtual void replay(fd_map_t& open_fds) {
        EnterWriteRec* enter = (EnterWriteRec*) other_half;

        ExitWriteRec*   exit = (ExitWriteRec*)  this;
    ;
    ;
    ;
     ssize_t  replay_ret =  write(open_fds[enter->fd], enter->buf, enter->count);
    if(replay_ret != ret) xi_warn("return mismatch: replay_ret=%d, ret=%d", (int)replay_ret, (int)ret);
;
    ;
    ;
    ;
    ;
    }

};

    
class SyscallRecParser: public RecParser {
    SyscallRec* find_enter_rec(const char* &off) {
        int enter_lsn;
        unmarshal(off, enter_lsn);
        assert(exists(enter_lsn, _syscalls));
        return _syscalls[enter_lsn];
    }
    
    Sgi::hash_map<int, SyscallRec*> _syscalls;
    
public:
    virtual void operator()(rec_type_t t, const char* &off, 
                            Log* l) {

        SyscallRec *enter;
        SyscallRec *exit;

        switch(t) {
            
            case EKM_enter_chdir:
            enter = new EnterChdirRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_close:
            enter = new EnterCloseRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_fdatasync:
            enter = new EnterFdatasyncRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_fsync:
            enter = new EnterFsyncRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_ftruncate:
            enter = new EnterFtruncateRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_link:
            enter = new EnterLinkRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_llseek:
            enter = new EnterLlseekRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_lseek:
            enter = new EnterLseekRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_mkdir:
            enter = new EnterMkdirRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_open:
            enter = new EnterOpenRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_pwrite64:
            enter = new EnterPwrite64Rec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_read:
            enter = new EnterReadRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_rename:
            enter = new EnterRenameRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_rmdir:
            enter = new EnterRmdirRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_symlink:
            enter = new EnterSymlinkRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_sync:
            enter = new EnterSyncRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_truncate:
            enter = new EnterTruncateRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_unlink:
            enter = new EnterUnlinkRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
    
            case EKM_enter_write:
            enter = new EnterWriteRec(off);
            _syscalls[enter->lsn()] = enter;
            l->push_back(enter);
            vn_out(4) << "parsing " << *enter << "\n"; 
            return;
            
            case EKM_exit_chdir:
            exit = new ExitChdirRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_close:
            exit = new ExitCloseRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_fdatasync:
            exit = new ExitFdatasyncRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_fsync:
            exit = new ExitFsyncRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_ftruncate:
            exit = new ExitFtruncateRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_link:
            exit = new ExitLinkRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_llseek:
            exit = new ExitLlseekRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_lseek:
            exit = new ExitLseekRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_mkdir:
            exit = new ExitMkdirRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_open:
            exit = new ExitOpenRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_pwrite64:
            exit = new ExitPwrite64Rec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_read:
            exit = new ExitReadRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_rename:
            exit = new ExitRenameRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_rmdir:
            exit = new ExitRmdirRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_symlink:
            exit = new ExitSymlinkRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_sync:
            exit = new ExitSyncRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_truncate:
            exit = new ExitTruncateRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_unlink:
            exit = new ExitUnlinkRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
    
            case EKM_exit_write:
            exit = new ExitWriteRec(off);
            enter = find_enter_rec(off);
            exit->pair(enter);
            enter->pair(exit);
            l->push_back(exit);
            vn_out(4) << "parsing " << *exit << "\
";
            return;
        }
    }

    void start(Log* l) {
    }

    void finish(Log* l) {
    }

    static void reg(void) {
        LogMgr::the()->reg_existing_category(EKM_SYSCALL_CAT, "syscall",
                                             new SyscallRecParser);
        
            LogMgr::the()->reg_existing_type(EKM_enter_chdir, "enter_chdir");
            LogMgr::the()->reg_existing_type(EKM_exit_chdir, "exit_chdir");
    
            LogMgr::the()->reg_existing_type(EKM_enter_close, "enter_close");
            LogMgr::the()->reg_existing_type(EKM_exit_close, "exit_close");
    
            LogMgr::the()->reg_existing_type(EKM_enter_fdatasync, "enter_fdatasync");
            LogMgr::the()->reg_existing_type(EKM_exit_fdatasync, "exit_fdatasync");
    
            LogMgr::the()->reg_existing_type(EKM_enter_fsync, "enter_fsync");
            LogMgr::the()->reg_existing_type(EKM_exit_fsync, "exit_fsync");
    
            LogMgr::the()->reg_existing_type(EKM_enter_ftruncate, "enter_ftruncate");
            LogMgr::the()->reg_existing_type(EKM_exit_ftruncate, "exit_ftruncate");
    
            LogMgr::the()->reg_existing_type(EKM_enter_link, "enter_link");
            LogMgr::the()->reg_existing_type(EKM_exit_link, "exit_link");
    
            LogMgr::the()->reg_existing_type(EKM_enter_llseek, "enter_llseek");
            LogMgr::the()->reg_existing_type(EKM_exit_llseek, "exit_llseek");
    
            LogMgr::the()->reg_existing_type(EKM_enter_lseek, "enter_lseek");
            LogMgr::the()->reg_existing_type(EKM_exit_lseek, "exit_lseek");
    
            LogMgr::the()->reg_existing_type(EKM_enter_mkdir, "enter_mkdir");
            LogMgr::the()->reg_existing_type(EKM_exit_mkdir, "exit_mkdir");
    
            LogMgr::the()->reg_existing_type(EKM_enter_open, "enter_open");
            LogMgr::the()->reg_existing_type(EKM_exit_open, "exit_open");
    
            LogMgr::the()->reg_existing_type(EKM_enter_pwrite64, "enter_pwrite64");
            LogMgr::the()->reg_existing_type(EKM_exit_pwrite64, "exit_pwrite64");
    
            LogMgr::the()->reg_existing_type(EKM_enter_read, "enter_read");
            LogMgr::the()->reg_existing_type(EKM_exit_read, "exit_read");
    
            LogMgr::the()->reg_existing_type(EKM_enter_rename, "enter_rename");
            LogMgr::the()->reg_existing_type(EKM_exit_rename, "exit_rename");
    
            LogMgr::the()->reg_existing_type(EKM_enter_rmdir, "enter_rmdir");
            LogMgr::the()->reg_existing_type(EKM_exit_rmdir, "exit_rmdir");
    
            LogMgr::the()->reg_existing_type(EKM_enter_symlink, "enter_symlink");
            LogMgr::the()->reg_existing_type(EKM_exit_symlink, "exit_symlink");
    
            LogMgr::the()->reg_existing_type(EKM_enter_sync, "enter_sync");
            LogMgr::the()->reg_existing_type(EKM_exit_sync, "exit_sync");
    
            LogMgr::the()->reg_existing_type(EKM_enter_truncate, "enter_truncate");
            LogMgr::the()->reg_existing_type(EKM_exit_truncate, "exit_truncate");
    
            LogMgr::the()->reg_existing_type(EKM_enter_unlink, "enter_unlink");
            LogMgr::the()->reg_existing_type(EKM_exit_unlink, "exit_unlink");
    
            LogMgr::the()->reg_existing_type(EKM_enter_write, "enter_write");
            LogMgr::the()->reg_existing_type(EKM_exit_write, "exit_write");
    }
};

/* END SYSCALL */

#endif //#ifndef __LOG_H
