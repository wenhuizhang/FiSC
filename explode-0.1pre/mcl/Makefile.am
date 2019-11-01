noinst_LIBRARIES = libmc.a 
SUBDIRS = 
SUBDIR_OBJS = $(SUBDIRS:%=%/obj.o)

libmc_a_SOURCES = util.C options.C lookup2.c mcl-options.C \
	  	  Archive.C State.C StateQueue.C ModelChecker.C Random.C \
	          helper.C Monitor.C mcl_main.C random.c \
		  Archive.h mcl.h ModelChecker.h output.h State.h \
		  hash_map_compat.h mcl_types.h Monitor.h random.h \
		  StateQueue.h lookup2.h options.h Random.h util.h \
	          mcl-options.h 

sbin_PROGRAMS = monitor

# remote model checking monitor
monitor_SOURCES = monitor_main.C
monitor_LDADD = $(LIBMC)

.PHONY: oh

oh: mcl.default.options
	cd $(srcdir) && sh ../gen-opt.pl mcl.default.options
