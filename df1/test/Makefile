# $Header: /home/cjm/cvs/frodospec/df1/test/Makefile,v 1.1 2023-03-21 14:36:52 cjm Exp $

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(FRODOSPEC_DF1_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(FRODOSPEC_DF1_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(FRODOSPEC_DF1_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR)
DOCFLAGS 	= -static

SRCS 		= df1_test_read_boolean.c df1_test_write_boolean.c df1_test_write_integer.c df1_test_read_integer.c \
		df1_test_read_float.c df1_test_write_float.c
OBJS 		= $(SRCS:%.c=%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)
LIBNAME         = $(FRODOSPEC_HOME)_$(FRODOSPEC_DF1_HOME)

top: $(PROGS) docs

$(BINDIR)/%: %.o
	cc -o $@ $< -L$(LT_LIB_HOME) -l$(LIBNAME) $(TIMELIB) $(SOCKETLIB) -lm -lc

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(PROGS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

backup: tidy
	$(RM) $(RM_OPTIONS) $(OBJS)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)

checkout:
	-$(CO) $(CO_OPTIONS) $(SRCS)

# DO NOT DELETE

df1_test_read_boolean.o: /usr/include/stdio.h /usr/include/features.h
df1_test_read_boolean.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_test_read_boolean.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_test_read_boolean.o: /usr/include/bits/types.h
df1_test_read_boolean.o: /usr/include/bits/wordsize.h
df1_test_read_boolean.o: /usr/include/bits/typesizes.h /usr/include/libio.h
df1_test_read_boolean.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_test_read_boolean.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1_test_read_boolean.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_test_read_boolean.o: /usr/include/bits/stdio_lim.h
df1_test_read_boolean.o: /usr/include/bits/sys_errlist.h
df1_test_read_boolean.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_test_read_boolean.o: /home/dev/src/frodospec/df1/include/df1.h
df1_test_read_boolean.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_test_read_boolean.o: /home/dev/src/frodospec/df1/include/df1_read_write.h
df1_test_read_boolean.o: /home/dev/src/frodospec/df1/include/df1_socket.h
df1_test_read_boolean.o: /home/dev/src/frodospec/df1/include/df1_serial.h
df1_test_read_boolean.o: /usr/include/termios.h /usr/include/bits/termios.h
df1_test_read_boolean.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
df1_test_read_boolean.o: /usr/include/bits/confname.h
df1_test_write_boolean.o: /usr/include/stdio.h /usr/include/features.h
df1_test_write_boolean.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_test_write_boolean.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_test_write_boolean.o: /usr/include/bits/types.h
df1_test_write_boolean.o: /usr/include/bits/wordsize.h
df1_test_write_boolean.o: /usr/include/bits/typesizes.h /usr/include/libio.h
df1_test_write_boolean.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_test_write_boolean.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1_test_write_boolean.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_test_write_boolean.o: /usr/include/bits/stdio_lim.h
df1_test_write_boolean.o: /usr/include/bits/sys_errlist.h
df1_test_write_boolean.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_test_write_boolean.o: /home/dev/src/frodospec/df1/include/df1.h
df1_test_write_boolean.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_test_write_boolean.o: /home/dev/src/frodospec/df1/include/df1_read_write.h
df1_test_write_boolean.o: /home/dev/src/frodospec/df1/include/df1_socket.h
df1_test_write_boolean.o: /home/dev/src/frodospec/df1/include/df1_serial.h
df1_test_write_boolean.o: /usr/include/termios.h /usr/include/bits/termios.h
df1_test_write_boolean.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
df1_test_write_boolean.o: /usr/include/bits/confname.h
