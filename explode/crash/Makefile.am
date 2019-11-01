# we cheat automake here saying we're going to create a noinst program, 
# then later we override the rule to generate obj.o
noinst_PROGRAMS = obj.o 
SUBDIRS =
SUBDIR_OBJS = $(patsubst %, %/obj.o, $(SUBDIRS))

obj_o_SOURCES =	Component.C BlockDev.C CrashSim.C ChunkDB.C Powerset.C \
	        Combination.C RawLog.C Cluster.C Fsck.C crash.C \
		CheckingContext.C BufferCache.C ReplayMgr.C \
	        Permuter.C SyscallPermuter.C Log.C LogMgr.C GenericDev.C\
                BufferCache.h CheckingContext.h crash-options.C\
                ChunkDB.h Cluster.h Combination.h Component.h \
		composit_iterator.h crash.h CrashSim.h ekm_log_types.h \
		Fsck.h LinuxLog.h Log.h LogMgr.h marshal.h net_helpers.h \
		Permuter.h Permuter-interface.h Powerset.h RawLog.h \
		Replayer.h ReplayMgr.h sector_loc.h unmarshal.h \
		SyscallPermuter.h GenericDev.h BlockDev.h crash-options.h

obj.o: $(SUBDIR_OBJS) $(obj_o_OBJECTS)
	$(LD) -r -o obj.o $(SUBDIR_OBJS) $(obj_o_OBJECTS) 

# finally, the crash lib
noinst_LIBRARIES = libcrash.a
libcrash_a_SOURCES =
libcrash_a_LIBADD = $(top_builddir)/rpc/obj.o $(top_builddir)/driver/obj.o \
		    $(top_builddir)/storage/obj.o obj.o

#sbin_PROGRAMS = fisc_net_server
#fisc_net_server_SOURCES = fisc_net_server.C dummy_assemble.C
#fisc_net_server_DEPENDENCIES = $(LIBCRASH)
#fisc_net_server_LDADD = $(LIBCRASH)

.PHONY: pih

pih:
	cd $(srcdir) && sh ./Permuter-interface.h > Permuter-interface.h~ \
		&& mv -f Permuter-interface.h~ Permuter-interface.h

oh: crash.default.options
	cd $(srcdir) && sh ../gen-opt.pl crash.default.options
