# $Header: /home/cjm/cvs/frodospec/ccd/test/Makefile,v 1.4 2000-01-24 10:25:23 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

INCDIR = $(CCD_SRC_HOME)/include

CFLAGS = -g -I$(INCDIR) -I$(CFITSIOINCDIR) -c

SRCS = test.c
OBJS		=	$(SRCS:%.c=%.o)

top: $(CCD_BIN_HOME)/test/test

$(CCD_BIN_HOME)/test/test: test.o
	cc -o $@ test.o -L$(LT_LIB_HOME) -lccd -lcfitsio $(TIMELIB) -lsocket -lm -lc

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
