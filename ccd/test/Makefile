# $Header: /home/cjm/cvs/frodospec/ccd/test/Makefile,v 1.6 2001-01-18 10:49:53 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

INCDIR = $(CCD_SRC_HOME)/include

CFLAGS = -g -I$(INCDIR) -I$(CFITSIOINCDIR) -c

SRCS = test.c test_filter_wheel.c ccd_read_memory.c ccd_write_memory.c
OBJS = $(SRCS:%.c=%.o)
PROGS = $(SRCS:%.c=$(CCD_BIN_HOME)/test/%)

top: $(PROGS)

$(CCD_BIN_HOME)/test/test: test.o
	cc -o $@ test.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(CCD_BIN_HOME)/test/test_filter_wheel: test_filter_wheel.o
	cc -o $@ test_filter_wheel.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(CCD_BIN_HOME)/test/ccd_read_memory: ccd_read_memory.o
	cc -o $@ ccd_read_memory.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

$(CCD_BIN_HOME)/test/ccd_write_memory: ccd_write_memory.o
	cc -o $@ ccd_write_memory.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

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
test.o: ../include/ccd_setup.h ../include/ccd_exposure.h
test.o: ../include/ccd_global.h ../include/ccd_interface.h
test.o: ../include/ccd_dsp.h ../include/ccd_text.h /usr/include/sys/types.h
test.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
test.o: /usr/include/sys/select.h /usr/include/sys/time.h
test.o: /usr/include/sys/time.h ../include/ccd_temperature.h
