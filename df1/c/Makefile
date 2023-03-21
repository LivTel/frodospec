# $Header: /home/cjm/cvs/frodospec/df1/c/Makefile,v 1.1 2023-03-21 14:34:10 cjm Exp $

include			../../../Makefile.common
include			../../Makefile.common
include			../Makefile.common

INCDIR = $(FRODOSPEC_DF1_SRC_HOME)/include
DOCSDIR = $(FRODOSPEC_DF1_DOC_HOME)/cdocs

# Do we want to enable logging?
# no logging
#LOGGINGCFLAGS = -DLOGGING=0
LOGGINGCFLAGS = -DLOGGING=10

#Do we want MUTEX protected locking around controller command communication?
MUTEXCFLAGS = -DDF1_MUTEXED=1
#MUTEXCFLAGS = 

CFLAGS = -g $(CCHECKFLAG) -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR) -L$(LT_LIB_HOME) \
	$(MUTEXCFLAGS) $(LOGGINGCFLAGS)

LINTFLAGS = -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR)
DOCFLAGS = -static
SRCS 		= 	df1_general.c df1_serial.c df1_socket.c df1_interface.c df1.c df1_read_write.c
HEADERS		=	$(SRCS:%.c=%.h)
OBJS		=	$(SRCS:%.c=%.o)
DOCS 		= 	$(SRCS:%.c=$(DOCSDIR)/%.html)
JAVASRCS 	= 	$(SRCS) ngat_frodospec_df1_Df1Library.c
JAVAOBJS	=	$(JAVASRCS:%.c=%.o)
JAVADOCS 	= 	$(JAVASRCS:%.c=$(DOCSDIR)/%.html)
LIBNAME		=	lib$(FRODOSPEC_HOME)_$(FRODOSPEC_DF1_HOME)

top: $(INCDIR)/ngat_frodospec_df1_Df1Library.h shared docs 

shared: $(LT_LIB_HOME)/$(LIBNAME).so

$(LT_LIB_HOME)/$(LIBNAME).so: $(JAVAOBJS)
	cc $(CCSHAREDFLAG) $(CFLAGS) $(JAVAOBJS) -o $@ $(TIMELIB) 

static: $(LT_LIB_HOME)/$(LIBNAME).a

$(LT_LIB_HOME)/$(LIBNAME).a: $(OBJS)
	ar rcv $@ $?

# special case for ngat_frodospec_ccd_CCDLibrary - depending on whether ngat.frodospec.ccd.CCDLibrary has changed
# recreate the machine generated JNI header
# touch it, javah does not update the timestamp correctly
# $(LT_JAVALIB_HOME)/ngat_frodospec.jar
$(INCDIR)/ngat_frodospec_df1_Df1Library.h: $(FRODOSPEC_SRC_HOME)/java/ngat/frodospec/df1/Df1Library.java
	$(JAVAH) -classpath $(FRODOSPEC_BIN_HOME)/java:$(CLASSPATH) -jni -d $(INCDIR) ngat.frodospec.df1.Df1Library
	$(TOUCH) $@

docs: $(JAVADOCS)

$(JAVADOCS): $(JAVASRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(JAVASRCS)

$(JAVADOCS) : $(JAVASRCS)

checkout:
	-$(CO) $(CO_OPTIONS) $(JAVASRCS)
	-(cd $(INCDIR); $(CO) $(CO_OPTIONS) $(HEADERS);)

checkin:
	-$(CI) $(CI_OPTIONS) $(JAVASRCS)
	-(cd $(INCDIR); $(CI) $(CI_OPTIONS) $(HEADERS);)

staticdepend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(JAVASRCS)

lint:
	$(LINT)	$(LINTFLAGS) $(JAVASRCS)

clean:
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_frodospec_df1_Df1Library.h $(LT_LIB_HOME)/$(LIBNAME).so $(LT_LIB_HOME)/$(LIBNAME)_g.so $(LT_LIB_HOME)/$(LIBNAME).a $(TIDY_OPTIONS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

backup: tidy
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_frodospec_df1_Df1Library.h

#
# $Log: not supported by cvs2svn $
#

# DO NOT DELETE

df1_general.o: /usr/include/errno.h /usr/include/features.h
df1_general.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_general.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
df1_general.o: /usr/include/asm/errno.h /usr/include/stdio.h
df1_general.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_general.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
df1_general.o: /usr/include/bits/typesizes.h /usr/include/libio.h
df1_general.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_general.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1_general.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_general.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
df1_general.o: /usr/include/stdlib.h /usr/include/string.h
df1_general.o: /usr/include/time.h /usr/include/bits/time.h
df1_general.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
df1_general.o: /usr/include/bits/confname.h /usr/include/getopt.h
df1_general.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_serial.o: /usr/include/errno.h /usr/include/features.h
df1_serial.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_serial.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
df1_serial.o: /usr/include/asm/errno.h /usr/include/stdio.h
df1_serial.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_serial.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
df1_serial.o: /usr/include/bits/typesizes.h /usr/include/libio.h
df1_serial.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_serial.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1_serial.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_serial.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
df1_serial.o: /usr/include/stdlib.h /usr/include/string.h
df1_serial.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
df1_serial.o: /usr/include/sys/types.h /usr/include/time.h
df1_serial.o: /usr/include/bits/time.h /usr/include/termios.h
df1_serial.o: /usr/include/bits/termios.h /usr/include/unistd.h
df1_serial.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
df1_serial.o: /usr/include/getopt.h
df1_serial.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_serial.o: /home/dev/src/frodospec/df1/include/df1_serial.h
df1_serial.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_socket.o: /usr/include/arpa/inet.h /usr/include/features.h
df1_socket.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_socket.o: /usr/include/netinet/in.h /usr/include/stdint.h
df1_socket.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
df1_socket.o: /usr/include/bits/types.h
df1_socket.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_socket.o: /usr/include/bits/typesizes.h /usr/include/bits/socket.h
df1_socket.o: /usr/include/limits.h
df1_socket.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/limits.h
df1_socket.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
df1_socket.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
df1_socket.o: /usr/include/sys/types.h /usr/include/time.h
df1_socket.o: /usr/include/bits/time.h /usr/include/bits/sockaddr.h
df1_socket.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
df1_socket.o: /usr/include/bits/in.h /usr/include/endian.h
df1_socket.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
df1_socket.o: /usr/include/errno.h /usr/include/bits/errno.h
df1_socket.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
df1_socket.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
df1_socket.o: /usr/include/netdb.h /usr/include/bits/netdb.h
df1_socket.o: /usr/include/stdio.h /usr/include/libio.h
df1_socket.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_socket.o: /usr/include/gconv.h
df1_socket.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_socket.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
df1_socket.o: /usr/include/stdlib.h /usr/include/string.h
df1_socket.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
df1_socket.o: /usr/include/bits/uio.h /usr/include/termios.h
df1_socket.o: /usr/include/bits/termios.h /usr/include/unistd.h
df1_socket.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
df1_socket.o: /usr/include/getopt.h
df1_socket.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_socket.o: /home/dev/src/frodospec/df1/include/df1_socket.h
df1_socket.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_interface.o: /usr/include/errno.h /usr/include/features.h
df1_interface.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_interface.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
df1_interface.o: /usr/include/asm/errno.h /usr/include/stdio.h
df1_interface.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_interface.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
df1_interface.o: /usr/include/bits/typesizes.h /usr/include/libio.h
df1_interface.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_interface.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1_interface.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_interface.o: /usr/include/bits/stdio_lim.h
df1_interface.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
df1_interface.o: /usr/include/string.h /usr/include/time.h
df1_interface.o: /usr/include/bits/time.h /usr/include/unistd.h
df1_interface.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
df1_interface.o: /usr/include/getopt.h /usr/include/pthread.h
df1_interface.o: /usr/include/sched.h /usr/include/bits/sched.h
df1_interface.o: /usr/include/signal.h /usr/include/bits/sigset.h
df1_interface.o: /usr/include/bits/pthreadtypes.h
df1_interface.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
df1_interface.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_interface.o: /home/dev/src/frodospec/df1/include/df1_serial.h
df1_interface.o: /usr/include/termios.h /usr/include/bits/termios.h
df1_interface.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_interface.o: /home/dev/src/frodospec/df1/include/df1_socket.h
df1.o: /usr/include/ctype.h /usr/include/features.h /usr/include/sys/cdefs.h
df1.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
df1.o: /usr/include/bits/wordsize.h
df1.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1.o: /usr/include/bits/typesizes.h /usr/include/endian.h
df1.o: /usr/include/bits/endian.h /usr/include/errno.h
df1.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
df1.o: /usr/include/asm/errno.h /usr/include/stdio.h /usr/include/libio.h
df1.o: /usr/include/_G_config.h /usr/include/wchar.h
df1.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
df1.o: /usr/include/stdlib.h /usr/include/string.h /usr/include/time.h
df1.o: /usr/include/bits/time.h /usr/include/unistd.h
df1.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
df1.o: /usr/include/getopt.h
df1.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1.o: /home/dev/src/frodospec/df1/include/df1.h
df1.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_read_write.o: /usr/include/errno.h /usr/include/features.h
df1_read_write.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
df1_read_write.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
df1_read_write.o: /usr/include/asm/errno.h /usr/include/stdio.h
df1_read_write.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
df1_read_write.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
df1_read_write.o: /usr/include/bits/typesizes.h /usr/include/libio.h
df1_read_write.o: /usr/include/_G_config.h /usr/include/wchar.h
df1_read_write.o: /usr/include/bits/wchar.h /usr/include/gconv.h
df1_read_write.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
df1_read_write.o: /usr/include/bits/stdio_lim.h
df1_read_write.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
df1_read_write.o: /usr/include/string.h /usr/include/time.h
df1_read_write.o: /usr/include/bits/time.h /usr/include/unistd.h
df1_read_write.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
df1_read_write.o: /usr/include/getopt.h
df1_read_write.o: /home/dev/src/frodospec/df1/include/df1_general.h
df1_read_write.o: /home/dev/src/frodospec/df1/include/df1.h
df1_read_write.o: /home/dev/src/frodospec/df1/include/df1_interface.h
df1_read_write.o: /home/dev/src/frodospec/df1/include/df1_read_write.h
