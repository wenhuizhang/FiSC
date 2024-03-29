/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from $FreeBSD: src/sys/tools/vnode_if.awk,v 1.50 2005/06/09 20:20:30 ssouhlal Exp $
 */


struct vop_islocked_args;
typedef int vop_islocked_t(struct vop_islocked_args *);

struct vop_lookup_args;
typedef int vop_lookup_t(struct vop_lookup_args *);

struct vop_cachedlookup_args;
typedef int vop_cachedlookup_t(struct vop_cachedlookup_args *);

struct vop_create_args;
typedef int vop_create_t(struct vop_create_args *);

struct vop_whiteout_args;
typedef int vop_whiteout_t(struct vop_whiteout_args *);

struct vop_mknod_args;
typedef int vop_mknod_t(struct vop_mknod_args *);

struct vop_open_args;
typedef int vop_open_t(struct vop_open_args *);

struct vop_close_args;
typedef int vop_close_t(struct vop_close_args *);

struct vop_access_args;
typedef int vop_access_t(struct vop_access_args *);

struct vop_getattr_args;
typedef int vop_getattr_t(struct vop_getattr_args *);

struct vop_setattr_args;
typedef int vop_setattr_t(struct vop_setattr_args *);

struct vop_read_args;
typedef int vop_read_t(struct vop_read_args *);

struct vop_write_args;
typedef int vop_write_t(struct vop_write_args *);

struct vop_lease_args;
typedef int vop_lease_t(struct vop_lease_args *);

struct vop_ioctl_args;
typedef int vop_ioctl_t(struct vop_ioctl_args *);

struct vop_poll_args;
typedef int vop_poll_t(struct vop_poll_args *);

struct vop_kqfilter_args;
typedef int vop_kqfilter_t(struct vop_kqfilter_args *);

struct vop_revoke_args;
typedef int vop_revoke_t(struct vop_revoke_args *);

struct vop_fsync_args;
typedef int vop_fsync_t(struct vop_fsync_args *);

struct vop_remove_args;
typedef int vop_remove_t(struct vop_remove_args *);

struct vop_link_args;
typedef int vop_link_t(struct vop_link_args *);

struct vop_rename_args;
typedef int vop_rename_t(struct vop_rename_args *);

struct vop_mkdir_args;
typedef int vop_mkdir_t(struct vop_mkdir_args *);

struct vop_rmdir_args;
typedef int vop_rmdir_t(struct vop_rmdir_args *);

struct vop_symlink_args;
typedef int vop_symlink_t(struct vop_symlink_args *);

struct vop_readdir_args;
typedef int vop_readdir_t(struct vop_readdir_args *);

struct vop_readlink_args;
typedef int vop_readlink_t(struct vop_readlink_args *);

struct vop_inactive_args;
typedef int vop_inactive_t(struct vop_inactive_args *);

struct vop_reclaim_args;
typedef int vop_reclaim_t(struct vop_reclaim_args *);

struct vop_lock_args;
typedef int vop_lock_t(struct vop_lock_args *);

struct vop_unlock_args;
typedef int vop_unlock_t(struct vop_unlock_args *);

struct vop_bmap_args;
typedef int vop_bmap_t(struct vop_bmap_args *);

struct vop_strategy_args;
typedef int vop_strategy_t(struct vop_strategy_args *);

struct vop_getwritemount_args;
typedef int vop_getwritemount_t(struct vop_getwritemount_args *);

struct vop_print_args;
typedef int vop_print_t(struct vop_print_args *);

struct vop_pathconf_args;
typedef int vop_pathconf_t(struct vop_pathconf_args *);

struct vop_advlock_args;
typedef int vop_advlock_t(struct vop_advlock_args *);

struct vop_reallocblks_args;
typedef int vop_reallocblks_t(struct vop_reallocblks_args *);

struct vop_getpages_args;
typedef int vop_getpages_t(struct vop_getpages_args *);

struct vop_putpages_args;
typedef int vop_putpages_t(struct vop_putpages_args *);

struct vop_getacl_args;
typedef int vop_getacl_t(struct vop_getacl_args *);

struct vop_setacl_args;
typedef int vop_setacl_t(struct vop_setacl_args *);

struct vop_aclcheck_args;
typedef int vop_aclcheck_t(struct vop_aclcheck_args *);

struct vop_closeextattr_args;
typedef int vop_closeextattr_t(struct vop_closeextattr_args *);

struct vop_getextattr_args;
typedef int vop_getextattr_t(struct vop_getextattr_args *);

struct vop_listextattr_args;
typedef int vop_listextattr_t(struct vop_listextattr_args *);

struct vop_openextattr_args;
typedef int vop_openextattr_t(struct vop_openextattr_args *);

struct vop_deleteextattr_args;
typedef int vop_deleteextattr_t(struct vop_deleteextattr_args *);

struct vop_setextattr_args;
typedef int vop_setextattr_t(struct vop_setextattr_args *);

struct vop_setlabel_args;
typedef int vop_setlabel_t(struct vop_setlabel_args *);

