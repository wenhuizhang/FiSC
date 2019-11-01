# we cheat here, then override the rule to generate obj.o later
noinst_PROGRAMS = obj.o 
SUBDIRS =
SUBDIR_OBJS = $(SUBDIRS:%=%/obj.o)
obj_o_SOURCES = bisim.h writes.h TreeCanon.h driver-options.C driver-options.h\
		TreeCanon.C writes.C data.C AbstFS.C StableFS.C FsTestDriver.C\
		AbstFS.h  data.h    StableFS.h    TreeCanon.h \
		bisim.h   FsTestDriver.h  TestDriver.h       writes.h


obj.o: $(SUBDIR_OBJS) $(obj_o_OBJECTS)
	$(LD) -r -o obj.o $(SUBDIR_OBJS) $(obj_o_OBJECTS) 

oh: driver.default.options
	cd $(srcdir) && sh ../gen-opt.pl driver.default.options
