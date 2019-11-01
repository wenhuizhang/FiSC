# we cheat here, then override the rule to generate obj.o later
noinst_PROGRAMS = obj.o 
SUBDIRS =
SUBDIR_OBJS = $(SUBDIRS:%=%/obj.o)
obj_o_SOURCES = Fs.C Raid.C storage-options.C\
	        Cvs.h  Fs.h  Nfs.h  Raid.h  Svn.h  VMWare.h storage-options.h

obj.o: $(SUBDIR_OBJS) $(obj_o_OBJECTS)
	$(LD) -r -o obj.o $(SUBDIR_OBJS) $(obj_o_OBJECTS) 

oh: storage.default.options
	cd $(srcdir) && sh ../gen-opt.pl storage.default.options
