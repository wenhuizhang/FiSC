#include <linux/module.h>
#include <linux/explode.h>
#include <linux/fs.h>
#include <linux/blkdev.h>

// all fields should initialized to NULL
struct explode_kernel_hooks ek_hooks = {0}; 
spinlock_t ek_hooks_lock = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL(ek_hooks);
EXPORT_SYMBOL(ek_hooks_lock);

/* Returns the path to take, out of N_POSSIBILITIES
   possibilities, at choice point CP.  The default choice point
   is always 0. */
int
choose(enum choice_point cp, int n_possibilities) 
{
	if(ek_hooks.choose)
		return ek_hooks.choose(cp, n_possibilities);
	return 0;
}

/* must call disable_choose first, then call enable_choose */
static int choices_enabled = 0;  /* == 0 means enabled */
void
enable_choose(void)
{
	choices_enabled ++;
	if(choices_enabled > 0)
		panic("EXPLODE: enable_choose(%d) > 0!", choices_enabled);
}

void
disable_choose(void)
{
	if(choices_enabled > 0)
		panic("EXPLODE: disable_choose(%d) > 0!", choices_enabled);
        choices_enabled --;
}

int choose_enabled(void)
{
	return choices_enabled == 0;
}

int fail_req(struct request *req)
{
	if(ek_hooks.fail_req)
		return ek_hooks.fail_req(req);
	return 0;
}

#if 0
#define EKM_SYSCALL(fn, rettype, args...) \
rettype explode_#fn(args) { \
EXPLODE_SYSCALL_INIT;
EXPLODE_SYSCALL_BEFOREINIT;
}
#include <linux/explode_def.h>
#undef EKM_SYSCALL
#endif

EXPORT_SYMBOL(choose);
EXPORT_SYMBOL(enable_choose);
EXPORT_SYMBOL(disable_choose);
EXPORT_SYMBOL(choose_enabled);
EXPORT_SYMBOL(fail_req);
