// DO NOT EDIT -- automatically generated by ../gen-opt.pl
// from driver.default.options


// stop being C compatible, to simplify things.  The entire code base 
// is in C++ now, except the kernel drivers.  Can always create a C interface 
// if need be.

#ifndef __driver_OPTIONS_H
#define __driver_OPTIONS_H

extern int __check__fsync;
extern int __check__fsync_parents;
extern int __check__ignore_missing;
extern int __check__journal_data;
extern int __check__mount_dir_sync;
extern int __check__mount_sync;
extern int __check__o_sync;
extern int __check__sync;
extern int __check__umount;
extern const char* __fstest__mutate;
extern int __fstest__name_len;
extern int __fstest__num_ops;
extern int __stablefs__check_data_loss_only;
extern int __stablefs__check_metadata_only;
extern int __stablefs__sync;


#endif
