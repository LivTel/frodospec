# $Header: /home/cjm/cvs/frodospec/ccd/test/Makefile,v 1.9 2002-11-07 19:23:31 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(CCD_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(CCD_BIN_HOME)/$(TESTDIR)
DOCSDIR 	= $(CCD_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) -I$(CFITSIOINCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR) -c
DOCFLAGS 	= -static

SRCS 		= test.c test_filter_wheel.c ccd_read_memory.c ccd_write_memory.c posix_time.c time_millis.c \
			ccd_read_memory_17.c ccd_test_data_link_17.c test_dsp_download.c test_reset_controller.c \
			test_data_link.c test_idle_clocking.c test_analogue_power.c test_temperature.c \
			test_setup_startup.c test_setup_dimensions.c test_setup_shutdown.c test_exposure.c \
			test_shutter.c test_abort.c
OBJS 		= $(SRCS:%.c=%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: $(PROGS) docs

$(BINDIR)/test: test.o
	cc -o $@ test.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_filter_wheel: test_filter_wheel.o
	cc -o $@ test_filter_wheel.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/ccd_read_memory: ccd_read_memory.o
	cc -o $@ ccd_read_memory.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/ccd_write_memory: ccd_write_memory.o
	cc -o $@ ccd_write_memory.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_dsp_download: test_dsp_download.o
	cc -o $@ test_dsp_download.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_reset_controller: test_reset_controller.o
	cc -o $@ test_reset_controller.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_data_link: test_data_link.o
	cc -o $@ test_data_link.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_idle_clocking: test_idle_clocking.o
	cc -o $@ test_idle_clocking.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_analogue_power: test_analogue_power.o
	cc -o $@ test_analogue_power.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_temperature: test_temperature.o
	cc -o $@ test_temperature.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_setup_startup: test_setup_startup.o
	cc -o $@ test_setup_startup.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_setup_dimensions: test_setup_dimensions.o
	cc -o $@ test_setup_dimensions.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_setup_shutdown: test_setup_shutdown.o
	cc -o $@ test_setup_shutdown.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_exposure: test_exposure.o
	cc -o $@ test_exposure.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_shutter: test_shutter.o
	cc -o $@ test_shutter.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/test_abort: test_abort.o
	cc -o $@ test_abort.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(BINDIR)/ccd_read_memory_17: ccd_read_memory_17.o
	cc -o $@ ccd_read_memory_17.o -lm -lc

$(BINDIR)/ccd_test_data_link_17: ccd_test_data_link_17.o
	cc -o $@ ccd_test_data_link_17.o -lm -lc

$(BINDIR)/posix_time: posix_time.o
	cc -o $@ posix_time.o -L$(LT_LIB_HOME) $(TIMELIB) -lm -lc

$(BINDIR)/time_millis: time_millis.o
	cc -o $@ time_millis.o -L$(LT_LIB_HOME) $(TIMELIB) -lm -lc

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(CCD_BIN_HOME)/test/test $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

backup: tidy
	$(RM) $(RM_OPTIONS) $(OBJS)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)

checkout:
	-$(CO) $(CO_OPTIONS) $(SRCS)

# DO NOT DELETE

test.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test.o: /usr/include/time.h /usr/include/sys/types.h
test.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
test.o: /usr/include/sys/select.h /usr/include/sys/time.h
test.o: /space/home/dev/src/ccd/include/ccd_setup.h
test.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test.o: /space/home/dev/src/ccd/include/ccd_global.h
test.o: /space/home/dev/src/ccd/include/ccd_interface.h
test.o: /space/home/dev/src/ccd/include/ccd_exposure.h
test.o: /space/home/dev/src/ccd/include/ccd_text.h
test.o: /space/home/dev/src/ccd/include/ccd_temperature.h
test.o: /space/home/dev/src/cfitsio/fitsio.h /usr/include/stddef.h
test.o: /space/home/dev/src/cfitsio/longnam.h
test_filter_wheel.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test_filter_wheel.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test_filter_wheel.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test_filter_wheel.o: /usr/include/time.h /usr/include/sys/types.h
test_filter_wheel.o: /usr/include/sys/machtypes.h
test_filter_wheel.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
test_filter_wheel.o: /usr/include/sys/time.h
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_global.h
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_text.h
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_filter_wheel.h
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_setup.h
ccd_read_memory.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_read_memory.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_read_memory.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_read_memory.o: /usr/include/time.h /usr/include/sys/types.h
ccd_read_memory.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
ccd_read_memory.o: /usr/include/sys/select.h /usr/include/sys/time.h
ccd_read_memory.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_read_memory.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_read_memory.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_read_memory.o: /space/home/dev/src/ccd/include/ccd_text.h
ccd_read_memory.o: /space/home/dev/src/ccd/include/ccd_temperature.h
ccd_read_memory.o: /space/home/dev/src/ccd/include/ccd_setup.h
ccd_write_memory.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_write_memory.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_write_memory.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_write_memory.o: /usr/include/time.h /usr/include/sys/types.h
ccd_write_memory.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
ccd_write_memory.o: /usr/include/sys/select.h /usr/include/sys/time.h
ccd_write_memory.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_write_memory.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_write_memory.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_write_memory.o: /space/home/dev/src/ccd/include/ccd_text.h
posix_time.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
posix_time.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
posix_time.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
posix_time.o: /usr/include/errno.h /usr/include/sys/errno.h
posix_time.o: /usr/include/time.h /usr/include/sys/types.h
posix_time.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
posix_time.o: /usr/include/sys/select.h /usr/include/sys/time.h
time_millis.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
time_millis.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
time_millis.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
time_millis.o: /usr/include/limits.h /usr/include/sys/int_limits.h
time_millis.o: /usr/include/errno.h /usr/include/sys/errno.h
time_millis.o: /usr/include/time.h /usr/include/sys/types.h
time_millis.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
time_millis.o: /usr/include/sys/select.h /usr/include/sys/time.h
time_millis.o: /usr/java1.2/include/jni.h /usr/include/stdarg.h
time_millis.o: /usr/java1.2/include/solaris/jni_md.h
ccd_read_memory_17.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_read_memory_17.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_read_memory_17.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_read_memory_17.o: /usr/include/time.h /usr/include/sys/types.h
ccd_read_memory_17.o: /usr/include/sys/machtypes.h
ccd_read_memory_17.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_read_memory_17.o: /usr/include/sys/time.h /usr/include/errno.h
ccd_read_memory_17.o: /usr/include/sys/errno.h /usr/include/fcntl.h
ccd_read_memory_17.o: /usr/include/sys/fcntl.h
ccd_test_data_link_17.o: /usr/include/stdio.h
ccd_test_data_link_17.o: /usr/include/sys/feature_tests.h
ccd_test_data_link_17.o: /usr/include/sys/isa_defs.h
ccd_test_data_link_17.o: /usr/include/sys/va_list.h /usr/include/stdio_tag.h
ccd_test_data_link_17.o: /usr/include/stdio_impl.h /usr/include/time.h
ccd_test_data_link_17.o: /usr/include/sys/types.h
ccd_test_data_link_17.o: /usr/include/sys/machtypes.h
ccd_test_data_link_17.o: /usr/include/sys/int_types.h
ccd_test_data_link_17.o: /usr/include/sys/select.h /usr/include/sys/time.h
ccd_test_data_link_17.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_test_data_link_17.o: /usr/include/fcntl.h /usr/include/sys/fcntl.h
test_dsp_download.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test_dsp_download.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test_dsp_download.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test_dsp_download.o: /usr/include/time.h /usr/include/sys/types.h
test_dsp_download.o: /usr/include/sys/machtypes.h
test_dsp_download.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
test_dsp_download.o: /usr/include/sys/time.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_global.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_dsp_download.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_text.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_temperature.h
test_dsp_download.o: /space/home/dev/src/ccd/include/ccd_setup.h
test_reset_controller.o: /usr/include/stdio.h
test_reset_controller.o: /usr/include/sys/feature_tests.h
test_reset_controller.o: /usr/include/sys/isa_defs.h
test_reset_controller.o: /usr/include/sys/va_list.h /usr/include/stdio_tag.h
test_reset_controller.o: /usr/include/stdio_impl.h /usr/include/time.h
test_reset_controller.o: /usr/include/sys/types.h
test_reset_controller.o: /usr/include/sys/machtypes.h
test_reset_controller.o: /usr/include/sys/int_types.h
test_reset_controller.o: /usr/include/sys/select.h /usr/include/sys/time.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_global.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_dsp_download.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_text.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_temperature.h
test_reset_controller.o: /space/home/dev/src/ccd/include/ccd_setup.h
test_data_link.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test_data_link.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test_data_link.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test_data_link.o: /usr/include/time.h /usr/include/sys/types.h
test_data_link.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
test_data_link.o: /usr/include/sys/select.h /usr/include/sys/time.h
test_data_link.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_data_link.o: /space/home/dev/src/ccd/include/ccd_global.h
test_data_link.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_data_link.o: /space/home/dev/src/ccd/include/ccd_text.h
test_idle_clocking.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test_idle_clocking.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test_idle_clocking.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test_idle_clocking.o: /usr/include/time.h /usr/include/sys/types.h
test_idle_clocking.o: /usr/include/sys/machtypes.h
test_idle_clocking.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
test_idle_clocking.o: /usr/include/sys/time.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_global.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_dsp_download.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_text.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_temperature.h
test_idle_clocking.o: /space/home/dev/src/ccd/include/ccd_setup.h
test_analogue_power.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test_analogue_power.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test_analogue_power.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test_analogue_power.o: /usr/include/time.h /usr/include/sys/types.h
test_analogue_power.o: /usr/include/sys/machtypes.h
test_analogue_power.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
test_analogue_power.o: /usr/include/sys/time.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_global.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_dsp_download.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_text.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_temperature.h
test_analogue_power.o: /space/home/dev/src/ccd/include/ccd_setup.h
test_temperature.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
test_temperature.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
test_temperature.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
test_temperature.o: /usr/include/time.h /usr/include/sys/types.h
test_temperature.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
test_temperature.o: /usr/include/sys/select.h /usr/include/sys/time.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_dsp.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_global.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_interface.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_dsp_download.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_text.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_temperature.h
test_temperature.o: /space/home/dev/src/ccd/include/ccd_setup.h
