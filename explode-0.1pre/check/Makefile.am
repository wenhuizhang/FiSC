LDADD = $(LIBCRASH) $(LIBMC) 

bin_PROGRAMS =  check-fs check-fs-paper check-raid-fs check-raid\
		check-cvs check-cvs-fs-raid \
		check-svn vm-ext3 test-vm-sync vm-snapshot\
		set_oom_adj check-cvs-thesis

if CHECK_BDB
    bin_PROGRAMS += check-berkeleydb
endif

check_fs_SOURCES = check-fs.C main.C
check_fs_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_fs_paper_SOURCES = check-fs-paper.C main.C
check_fs_paper_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_raid_fs_SOURCES = check-raid-fs.C main.C
check_raid_fs_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_raid_SOURCES = check-raid.C RaidTestDriver.C RaidTestDriver.h main.C
check_raid_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_cvs_SOURCES = check-cvs.C CvsTestDriver.C CvsTestDriver.h main.C
check_cvs_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_cvs_thesis_SOURCES = check-cvs-thesis.C main.C
check_cvs_thesis_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_cvs_fs_raid_SOURCES = check-cvs-fs-raid.C CvsTestDriver.C \
	                    CvsTestDriver.h main.C
check_cvs_fs_raid_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

check_svn_SOURCES = check-svn.C main.C
check_svn_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

# I put BerkeleyDB.C here not in storage/ because they require
# linking against db lib
check_berkeleydb_SOURCES = BerkeleyDB.h BerkeleyDB.C check-berkeleydb.C bdb-options.C main.C
check_berkeleydb_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 
check_berkeleydb_LDADD = $(LIBCRASH) $(LIBMC) $(DBLIB)

vm_ext3_SOURCES = vm-ext3.C main.C
vm_ext3_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

test_vm_sync_SOURCES = test-vm-sync.C
test_vm_sync_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

vm_snapshot_SOURCES = vm-snapshot.C main.C
vm_snapshot_DEPENDENCIES = $(LIBCRASH) $(LIBMC) 

sbin_PROGRAMS = explode_server
explode_server_SOURCES = $(top_srcdir)/rpc/explode_rpc_svc.c \
			 $(top_srcdir)/rpc/explode_rpc_server.C
explode_server_LDADD = $(LIBCRASH) $(LIBMC) 

# this program prevents the linux oom killer from killing us
set_oom_adj_LDADD =

EXTRA_DIST = persist-run.sh fisc_net_server.sh

all-local:
	-ln -f -s $(top_srcdir)/$(RDD)/rdd.ko
	-ln -f -s $(top_srcdir)/$(EKM)/ekm.ko
	-ln -f -s $(top_srcdir)/$(MTD)/smtd.ko

clean-local:
	-rm -f rdd.ko ekm.ko smtd.ko


oh: bdb.default.options
	cd $(srcdir) && sh ../gen-opt.pl bdb.default.options
