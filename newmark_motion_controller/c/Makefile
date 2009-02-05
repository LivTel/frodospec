# $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/c/Makefile,v 1.2 2009-02-05 11:41:17 cjm Exp $

include			../../../Makefile.common
include			../../Makefile.common
include			../Makefile.common

INCDIR = $(NEWMARK_SRC_HOME)/include
DOCSDIR = $(NEWMARK_DOC_HOME)/cdocs

# Do we want to enable logging?
# no logging
#LOGGINGCFLAGS = -DLOGGING=0
LOGGINGCFLAGS = -DLOGGING=10

#Do we want MUTEX protected locking around controller command communication?
# Probably use ARCOM_ESS_MUTEX ?
#MUTEXCFLAGS = -DNEWMARK_MUTEXED=1
#MUTEXCFLAGS = 

CFLAGS = -g $(CCHECKFLAG) -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR) -I$(ARCOM_ESS_INCDIR) -L$(LT_LIB_HOME) \
	$(MUTEXCFLAGS) $(LOGGINGCFLAGS) $(LOG_UDP_CFLAGS)

LINTFLAGS 	= -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR)
DOCFLAGS 	= -static
SRCS 		= 	newmark_general.c newmark_command.c
HEADERS		=	$(SRCS:%.c=%.h)
OBJS		=	$(SRCS:%.c=%.o)
DOCS 		= 	$(SRCS:%.c=$(DOCSDIR)/%.html)
JAVASRCS 	= 	$(SRCS) ngat_frodospec_newmark_Newmark.c
JAVAOBJS	=	$(JAVASRCS:%.c=%.o)
JAVADOCS 	= 	$(JAVASRCS:%.c=$(DOCSDIR)/%.html)
LIBNAME		=	lib$(NEWMARK_LIBNAME)

top: shared docs $(INCDIR)/ngat_frodospec_newmark_Newmark.h 

shared: $(LT_LIB_HOME)/$(LIBNAME).so

$(LT_LIB_HOME)/$(LIBNAME).so: $(JAVAOBJS)
	cc $(CCSHAREDFLAG) $(CFLAGS) $(JAVAOBJS) -o $@ -l$(ARCOM_ESS_LIBNAME) $(TIMELIB) 

static: $(LT_LIB_HOME)/$(LIBNAME).a

$(LT_LIB_HOME)/$(LIBNAME).a: $(OBJS)
	ar rcv $@ $?

# special case for ngat_frodospec_newmark_Newmark - depending on whether ngat.frodospec.newmark.Newmark has changed
# recreate the machine generated JNI header
# touch it, javah does not update the timestamp correctly
# $(LT_JAVALIB_HOME)/ngat_frodospec.jar
$(INCDIR)/ngat_frodospec_newmark_Newmark.h: $(FRODOSPEC_SRC_HOME)/java/ngat/frodospec/newmark/Newmark.java
	$(JAVAH) -classpath $(FRODOSPEC_BIN_HOME)/java:$(CLASSPATH) -jni -d $(INCDIR) ngat.frodospec.newmark.Newmark
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
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_frodospec_newmark_Newmark.h $(LT_LIB_HOME)/$(LIBNAME).so $(LT_LIB_HOME)/$(LIBNAME)_g.so $(LT_LIB_HOME)/$(LIBNAME).a $(TIDY_OPTIONS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

backup: tidy
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_frodospec_newmark_Newmark.h

#
# $Log: not supported by cvs2svn $
# Revision 1.1  2008/11/20 11:35:54  cjm
# Initial revision
#
#

# DO NOT DELETE

newmark_general.o: /usr/include/errno.h /usr/include/features.h
newmark_general.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
newmark_general.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
newmark_general.o: /usr/include/asm/errno.h /usr/include/stdio.h
newmark_general.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
newmark_general.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
newmark_general.o: /usr/include/bits/typesizes.h /usr/include/libio.h
newmark_general.o: /usr/include/_G_config.h /usr/include/wchar.h
newmark_general.o: /usr/include/bits/wchar.h /usr/include/gconv.h
newmark_general.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
newmark_general.o: /usr/include/bits/stdio_lim.h
newmark_general.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
newmark_general.o: /usr/include/string.h /usr/include/time.h
newmark_general.o: /usr/include/bits/time.h /usr/include/unistd.h
newmark_general.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
newmark_general.o: /usr/include/getopt.h
newmark_general.o: /home/dev/src/frodospec/newmark_motion_controller/include/newmark_general.h
newmark_command.o: /usr/include/errno.h /usr/include/features.h
newmark_command.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
newmark_command.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
newmark_command.o: /usr/include/asm/errno.h /usr/include/stdio.h
newmark_command.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
newmark_command.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
newmark_command.o: /usr/include/bits/typesizes.h /usr/include/libio.h
newmark_command.o: /usr/include/_G_config.h /usr/include/wchar.h
newmark_command.o: /usr/include/bits/wchar.h /usr/include/gconv.h
newmark_command.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
newmark_command.o: /usr/include/bits/stdio_lim.h
newmark_command.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
newmark_command.o: /usr/include/string.h /usr/include/time.h
newmark_command.o: /usr/include/bits/time.h /usr/include/unistd.h
newmark_command.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
newmark_command.o: /usr/include/getopt.h
newmark_command.o: /home/dev/src/frodospec/newmark_motion_controller/include/newmark_general.h
newmark_command.o: /home/dev/src/frodospec/newmark_motion_controller/include/newmark_command.h
newmark_command.o: /home/dev/src/arcom_ess/include/arcom_ess_interface.h
