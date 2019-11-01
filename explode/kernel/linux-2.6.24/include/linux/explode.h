#ifndef __LINUX_EXPLODE_H
#define __LINUX_EXPLODE_H

/* Choice point identifiers. */
enum choice_point {
	EKM_KMALLOC = 1,
	EKM_USERCOPY = 2,
	EKM_BREAD = 4,
	EKM_SCHEDULE = 8,
	EKM_CACHE_READ = 16,
	EKM_DISK_READ = 32,
	EKM_DISK_WRITE = 64,
};
int choose(enum choice_point cp, int n_possibilities);
void enable_choose(void);
void disable_choose(void);
int choose_enabled(void);

#include <linux/spinlock.h>

struct buffer_head;
struct bio;
struct request;
#define EKM_BH(state) \
	void (*bh_##state)(struct buffer_head *bh)
#define EKM_BIO(state) \
	void (*bio_##state)(struct bio *bio)
#define EKM_REQ(state) \
	void (*req_##state)(struct request *req)

struct explode_kernel_hooks {

	// choose
	int (*choose)(enum choice_point cp, int n_possibilities);
	int (*fail_req)(struct request *req);

        void (*log_buffer_record)(int, int, long, int, const char*);
        
        /* struct buffer_head record types */
        EKM_BH(dirty);
        EKM_BH(clean);
        EKM_BH(lock);
        EKM_BH(wait);
        EKM_BH(unlock);
        EKM_BH(write);
        EKM_BH(write_sync);
#       undef EKM_BH

        /* struct bio record types */
        EKM_BIO(start_write);
        EKM_BIO(wait);
#       undef EKM_BIO

        /* struct request record types */
        EKM_REQ(write);
        EKM_REQ(read);
        EKM_REQ(write_done);
#       undef EKM_REQ


        /* BEGIN SYSCALL */
/* 
 *   Everything between BEGIN SYSCALL and END SYSCALL 
 *   is automatically generated from ./scripts/gen_syscall_log.pl 
 */

        void (*enter_sys_chdir)(int *entry_lsn, const char __user * filename);
        void (*exit_sys_chdir)(int *entry_lsn,  long  ret, const char __user * filename);
        void (*enter_sys_close)(int *entry_lsn, unsigned int  fd);
        void (*exit_sys_close)(int *entry_lsn,  long  ret, unsigned int  fd);
        void (*enter_sys_fdatasync)(int *entry_lsn, unsigned int  fd);
        void (*exit_sys_fdatasync)(int *entry_lsn,  long  ret, unsigned int  fd);
        void (*enter_sys_fsync)(int *entry_lsn, unsigned int  fd);
        void (*exit_sys_fsync)(int *entry_lsn,  long  ret, unsigned int  fd);
        void (*enter_sys_ftruncate)(int *entry_lsn, unsigned int  fd, unsigned long  length);
        void (*exit_sys_ftruncate)(int *entry_lsn,  long  ret, unsigned int  fd, unsigned long  length);
        void (*enter_sys_link)(int *entry_lsn, const char  __user * oldname, const char  __user * newname);
        void (*exit_sys_link)(int *entry_lsn,  long  ret, const char  __user * oldname, const char  __user * newname);
        void (*enter_sys_llseek)(int *entry_lsn, unsigned int  fd, unsigned long  offset_high, unsigned long  offset_low,  loff_t  __user * result, unsigned int  origin);
        void (*exit_sys_llseek)(int *entry_lsn,  long  ret, unsigned int  fd, unsigned long  offset_high, unsigned long  offset_low,  loff_t  __user * result, unsigned int  origin);
        void (*enter_sys_lseek)(int *entry_lsn, unsigned int  fd, off_t  offset, unsigned int  origin);
        void (*exit_sys_lseek)(int *entry_lsn,  off_t  ret, unsigned int  fd, off_t  offset, unsigned int  origin);
        void (*enter_sys_mkdir)(int *entry_lsn, const char  __user * pathname, int  mode);
        void (*exit_sys_mkdir)(int *entry_lsn,  long  ret, const char  __user * pathname, int  mode);
        void (*enter_sys_open)(int *entry_lsn, const char  __user * filename, int  flags, int  mode);
        void (*exit_sys_open)(int *entry_lsn,  long  ret, const char  __user * filename, int  flags, int  mode);
        void (*enter_sys_pwrite64)(int *entry_lsn, unsigned int  fd,  const char  __user * buf, size_t  count, loff_t  pos);
        void (*exit_sys_pwrite64)(int *entry_lsn,  ssize_t  ret, unsigned int  fd,  const char  __user * buf, size_t  count, loff_t  pos);
        void (*enter_sys_read)(int *entry_lsn, unsigned int  fd,    char  __user * buf, size_t  count);
        void (*exit_sys_read)(int *entry_lsn,  ssize_t  ret, unsigned int  fd,    char  __user * buf, size_t  count);
        void (*enter_sys_rename)(int *entry_lsn, const char  __user * oldname, const char  __user * newname);
        void (*exit_sys_rename)(int *entry_lsn,  long  ret, const char  __user * oldname, const char  __user * newname);
        void (*enter_sys_rmdir)(int *entry_lsn, const char  __user * pathname);
        void (*exit_sys_rmdir)(int *entry_lsn,  long  ret, const char  __user * pathname);
        void (*enter_sys_symlink)(int *entry_lsn, const char  __user * oldname, const char  __user * newname);
        void (*exit_sys_symlink)(int *entry_lsn,  long  ret, const char  __user * oldname, const char  __user * newname);
        void (*enter_sys_sync)(int *entry_lsn);
        void (*exit_sys_sync)(int *entry_lsn,  long  ret);
        void (*enter_sys_truncate)(int *entry_lsn, const char  __user * filename, unsigned long  length);
        void (*exit_sys_truncate)(int *entry_lsn,  long  ret, const char  __user * filename, unsigned long  length);
        void (*enter_sys_unlink)(int *entry_lsn, const char  __user * pathname);
        void (*exit_sys_unlink)(int *entry_lsn,  long  ret, const char  __user * pathname);
        void (*enter_sys_write)(int *entry_lsn, unsigned int  fd,  const char  __user * buf, size_t  count);
        void (*exit_sys_write)(int *entry_lsn,  ssize_t  ret, unsigned int  fd,  const char  __user * buf, size_t  count);
        /* END SYSCALL */
};

extern struct buffer_head *ekm_commit_bh;
extern struct bio *ekm_commit_bio;

extern struct explode_kernel_hooks ek_hooks;
extern spinlock_t ek_hooks_lock;

#define EXPLODE_HOOK(fn, args...) \
	do { \
		if(ek_hooks.fn) \
			(ek_hooks.fn) (args); \
	} while(0)

#define EXPLODE_INVALID_LSN (-1)
#define EXPLODE_SYSCALL_INIT \
	int explode_lsn = EXPLODE_INVALID_LSN;

#define EXPLODE_SYSCALL_BEFORE(fn, args...) \
	do { \
		if(ek_hooks.enter_##fn) \
			(ek_hooks.enter_##fn) (&explode_lsn, ##args); \
	} while(0)

#define EXPLODE_SYSCALL_AFTER(fn, ret, args...) \
	do { \
		if(ek_hooks.exit_##fn) \
			(ek_hooks.exit_##fn) (&explode_lsn, ret, ##args); \
	} while(0)

int fail_req(struct request *req);

#endif
