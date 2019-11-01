SUBDIRS = mcl rpc storage driver crash check testsuite scripts
#DIST_SUBDIRS = $(SUBDIRS)

EKM_COMMON = ekm_ioctl.h ekm_log_buf.h ekm_log_buf.c
EKM_LINUX = Makefile.in ekm.h ekm_log.h ekm_choice.h ekm_thread.h \
	    ekm_sysdep.h ekm_main.c ekm_log.c ekm_choice.c ekm_thread.c
RDD_LINUX = Makefile.in rdd.h rdd.c
MTD_LINUX = Makefile.in smtd_log.h smtd.c smtd_log.c
EKM_BSD = Makefile \
	  vnode_if_newproto.h vnode_if.h vnode_if_typedef.h ekm.h \
          ekm_log.h ekm_choice.h ekm_thread.h ekm_log_generated.h \
	  ekm_choice.c ekm_log.c ekm_log_generated.c ekm_main.c \
	  ekm_thread.c ekm_log_user_generated.C
RDD_BSD = Makefile rdd.c rdd.h

EXTRA_DIST = sysincl.h README.student\
	     $(patsubst %, ekm-common/%, $(EKM_COMMON)) \
	     $(patsubst %, ekm-linux-2.6/%, $(EKM_LINUX)) \
	     $(patsubst %, rdd-linux-2.6/%, $(RDD_LINUX)) \
	     $(patsubst %, mtd-linux-2.6/%, $(MTD_LINUX)) \
	     $(patsubst %, ekm-bsd-6.0/%, $(EKM_BSD)) \
	     $(patsubst %, rdd-bsd-6.0/%, $(RDD_BSD)) \
	     $(wildcard kernel/patch/*.patch) $(wildcard explode.options)

all-local:
	make -C $(EKM) all
	make -C $(RDD) all
#	make -C $(MTD) all

clean-local:
	make -C $(EKM) clean
	make -C $(RDD) clean
#	make -C $(MTD) clean

.PHONY: ec
ec:
	./scripts/gen_syscall_log.pl $(KERNEL_DIR) ./ekm-linux-2.6

.PHONY: oh
oh:
	make -C mcl oh
	make -C crash oh
	make -C storage oh
	make -C driver oh
	make -C check oh
