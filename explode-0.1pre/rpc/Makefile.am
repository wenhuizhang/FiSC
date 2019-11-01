# we cheat automake here saying we're going to create a noinst program, 
# then later we override the rule to generate obj.o
noinst_PROGRAMS = obj.o #explode_server
SUBDIRS =
SUBDIR_OBJS = $(SUBDIRS:%=%/obj.o)
obj_o_SOURCES =	HostLib.C host_lib_impl.C NetLib.C \
	explode_rpc_clnt.c explode_rpc_xdr.c \
	ekm_lib.h  explode_rpc.h  HostLib.h  host_lib_impl.h  NetLib.h

# can't make them here because LIBCRASH isn't ready yet
#explode_server_SOURCES = explode_rpc_svc.c explode_rpc_server.C
#explode_server_LDADD = $(LIBCRASH) $(LIBMC)
#explode_server_DEPENDENCIES = $(LIBCRASH) $(LIBMC)

obj.o: $(SUBDIR_OBJS) $(obj_o_OBJECTS)
	$(LD) -r -o obj.o $(SUBDIR_OBJS) $(obj_o_OBJECTS) 
