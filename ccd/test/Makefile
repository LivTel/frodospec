# $Header: /home/cjm/cvs/frodospec/ccd/test/Makefile,v 1.1 1999-06-24 15:36:45 dev Exp $

include ../../Makefile.common
include ../Makefile.common

LIBDIR = ../lib
INCDIR = ../include

CFLAGS = -g -I$(INCDIR) -c

SRCS = test.c
OBJS		=	$(SRCS:%.c=%.o)

top: test

test: test.o $(LIBDIR)/libccd.a 
	cc -o test test.o $(LIBDIR)/libccd.a $(TIMELIB) -lm -lc

depend:
	makedepend -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

checkin:
	echo cannot checkin in this directory

checkout:
	echo cannot checkout in this directory

# DO NOT DELETE

test.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/features.h
test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
test.o: /usr/include/_G_config.h /usr/include/gnu/types.h
test.o: /usr/lib/gcc-lib/i386-redhat-linux/2.7.2.3/include/stddef.h
test.o: /usr/lib/gcc-lib/i386-redhat-linux/2.7.2.3/include/stdarg.h
test.o: /usr/include/stdio_lim.h ../include/ccd_setup.h
test.o: ../include/ccd_exposure.h ../include/ccd_global.h
test.o: ../include/ccd_dsp.h ../include/ccd_interface.h
test.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
test.o: /usr/include/bytesex.h /usr/include/sys/select.h
test.o: /usr/include/selectbits.h ../include/ccd_text.h
test.o: ../include/ccd_temperature.h
