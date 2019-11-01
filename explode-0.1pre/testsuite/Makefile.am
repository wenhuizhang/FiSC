LDADD = $(LIBCRASH) $(LIBMC)

noinst_PROGRAMS = start_trace stop_trace get_log_size empty_log enum xywalk \
	xywalk-ckpt net_test abstract #copy_log

start_trace_SOURCES = start_trace.C
start_trace_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
stop_trace_SOURCES = stop_trace.C
stop_trace_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
get_log_size_SOURCES = get_log_size.C
get_log_size_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
empty_log_SOURCES = empty_log.C
empty_log_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
#copy_log_SOURCES = copy_log.C
#copy_log_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
net_test_SOURCES = net_test.C
net_test_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
abstract_SOURCES = abstract.C
abstract_DEPENDENCIES=$(LIBCRASH) $(LIBMC)
enum_SOURCES = enum.C
enum_LDADD =  $(LIBMC)
enum_DEPENDENCIES=$(LIBMC)
xywalk_SOURCES = xywalk.C
xywalk_LDADD =  $(LIBMC)
xywalk_DEPENDENCIES=$(LIBMC)
xywalk_ckpt_SOURCES = xywalk-ckpt.C
xywalk_ckpt_LDADD =  $(LIBMC)
xywalk_ckpt_DEPENDENCIES=$(LIBMC)

all-local:
	-ln -f -s $(top_srcdir)/$(RDD)/rdd.ko
	-ln -f -s $(top_srcdir)/$(EKM)/ekm.ko

clean-local:
	-rm -f rdd.ko ekm.ko