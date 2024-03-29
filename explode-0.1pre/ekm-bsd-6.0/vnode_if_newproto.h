/*
 * This file is produced automatically.
 * Do not modify anything in here by hand.
 *
 * Created from $FreeBSD: src/sys/tools/vnode_if.awk,v 1.50 2005/06/09 20:20:30 ssouhlal Exp $
 */


struct vop_vector {
	struct vop_vector	*vop_default;
	vop_bypass_t	*vop_bypass;
	vop_islocked_t	*vop_islocked;
	vop_lookup_t	*vop_lookup;
	vop_cachedlookup_t	*vop_cachedlookup;
	vop_create_t	*vop_create;
	vop_whiteout_t	*vop_whiteout;
	vop_mknod_t	*vop_mknod;
	vop_open_t	*vop_open;
	vop_close_t	*vop_close;
	vop_access_t	*vop_access;
	vop_getattr_t	*vop_getattr;
	vop_setattr_t	*vop_setattr;
	vop_read_t	*vop_read;
	vop_write_t	*vop_write;
	vop_lease_t	*vop_lease;
	vop_ioctl_t	*vop_ioctl;
	vop_poll_t	*vop_poll;
	vop_kqfilter_t	*vop_kqfilter;
	vop_revoke_t	*vop_revoke;
	vop_fsync_t	*vop_fsync;
	vop_remove_t	*vop_remove;
	vop_link_t	*vop_link;
	vop_rename_t	*vop_rename;
	vop_mkdir_t	*vop_mkdir;
	vop_rmdir_t	*vop_rmdir;
	vop_symlink_t	*vop_symlink;
	vop_readdir_t	*vop_readdir;
	vop_readlink_t	*vop_readlink;
	vop_inactive_t	*vop_inactive;
	vop_reclaim_t	*vop_reclaim;
	vop_lock_t	*vop_lock;
	vop_unlock_t	*vop_unlock;
	vop_bmap_t	*vop_bmap;
	vop_strategy_t	*vop_strategy;
	vop_getwritemount_t	*vop_getwritemount;
	vop_print_t	*vop_print;
	vop_pathconf_t	*vop_pathconf;
	vop_advlock_t	*vop_advlock;
	vop_reallocblks_t	*vop_reallocblks;
	vop_getpages_t	*vop_getpages;
	vop_putpages_t	*vop_putpages;
	vop_getacl_t	*vop_getacl;
	vop_setacl_t	*vop_setacl;
	vop_aclcheck_t	*vop_aclcheck;
	vop_closeextattr_t	*vop_closeextattr;
	vop_getextattr_t	*vop_getextattr;
	vop_listextattr_t	*vop_listextattr;
	vop_openextattr_t	*vop_openextattr;
	vop_deleteextattr_t	*vop_deleteextattr;
	vop_setextattr_t	*vop_setextattr;
	vop_setlabel_t	*vop_setlabel;
};
