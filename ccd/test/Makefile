# $Header: /home/cjm/cvs/frodospec/ccd/test/Makefile,v 1.8 2001-02-01 10:45:49 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(CCD_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(CCD_BIN_HOME)/$(TESTDIR)
DOCSDIR 	= $(CCD_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) -I$(CFITSIOINCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR) -c
DOCFLAGS 	= -static

SRCS 		= test.c test_filter_wheel.c ccd_read_memory.c ccd_write_memory.c posix_time.c time_millis.c
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
test_filter_wheel.o: /space/home/dev/src/ccd/include/ccd_temperature.h
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
