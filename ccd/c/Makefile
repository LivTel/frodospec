# $Header: /home/cjm/cvs/frodospec/ccd/c/Makefile,v 1.2 2000-03-08 18:27:35 cjm Exp $

include			../../Makefile.common
include			../Makefile.common

INCDIR = $(CCD_SRC_HOME)/include
DOCSDIR = $(CCD_DOC_HOME)/cdocs

#Do we want MUTEX protected locking around controller command communication?
MUTEXCFLAGS = -DCCD_DSP_MUTEXED=1
#MUTEXCFLAGS = 

# Do we wish to link with CFITSIO routines?
FITSCFLAGS = -DCFITSIO=1 -I$(CFITSIOINCDIR) -L$(LT_LIB_HOME)
CFITSIOLIB = -lcfitsio

# FITSCFLAGS =
# CFITSIOLIB =

CFLAGS = -g $(CCHECKFLAG) -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR) $(FITSCFLAGS) $(MUTEXCFLAGS)

LINTFLAGS = -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR)
DOCFLAGS = -static
SRCS 		= 	ccd_interface.c ccd_pci.c ccd_text.c ccd_global.c ccd_dsp.c ccd_temperature.c ccd_setup.c ccd_exposure.c
HEADERS		=	$(SRCS:%.c=%.h)
OBJS		=	$(SRCS:%.c=%.o)
DOCS 		= 	$(SRCS:%.c=$(DOCSDIR)/%.html)
JAVASRCS 	= 	$(SRCS) ngat_ccd_CCDLibrary.c
JAVAOBJS	=	$(JAVASRCS:%.c=%.o)
JAVADOCS 	= 	$(JAVASRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/libccd.so

$(LT_LIB_HOME)/libccd.so: $(JAVAOBJS)
	cc $(CCSHAREDFLAG) $(CFLAGS) $(JAVAOBJS) -o $@ $(TIMELIB) $(CFITSIOLIB)

static: $(LT_LIB_HOME)/libccd.a

$(LT_LIB_HOME)/libccd.a: $(OBJS)
	ar rcv $@ $?

# special case for ngat_ccd_CCDLibrary - depending on whether ngat.ccd.CCDLibrary has changed
# recreate the machine generated JNI header
$(INCDIR)/ngat_ccd_CCDLibrary.h: $(LT_JAVALIB_HOME)/ngat.jar
	$(JAVAH) -jni -d $(INCDIR) ngat.ccd.CCDLibrary

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
	makedepend -- $(CFLAGS) -- $(SRCS)

depend:
	makedepend -- $(CFLAGS) -- $(JAVASRCS)

lint:
	$(LINT)	$(LINTFLAGS) $(JAVASRCS)

clean:
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_ccd_CCDLibrary.h $(LT_LIB_HOME)/libccd.so $(LT_LIB_HOME)/libccd_g.so $(LT_LIB_HOME)/libccd.a $(TIDY_OPTIONS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

backup: tidy
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_ccd_CCDLibrary.h

#
# $Log: not supported by cvs2svn $
# Revision 1.1  2000/02/08 18:12:33  cjm
# Initial revision
#
#

# DO NOT DELETE

ccd_interface.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_interface.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_interface.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_interface.o: /usr/include/string.h /usr/include/fcntl.h
ccd_interface.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ccd_interface.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_interface.o: /usr/include/sys/time.h /usr/include/sys/time.h
ccd_interface.o: /usr/include/sys/fcntl.h /usr/include/signal.h
ccd_interface.o: /usr/include/sys/signal.h /usr/include/sys/unistd.h
ccd_interface.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_interface.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_interface.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_interface.o: /space/home/dev/src/ccd/include/ccd_text.h
ccd_interface.o: /space/home/dev/src/ccd/include/ccd_pci.h
ccd_pci.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_pci.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_pci.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_pci.o: /usr/include/unistd.h /usr/include/sys/types.h
ccd_pci.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
ccd_pci.o: /usr/include/sys/select.h /usr/include/sys/time.h
ccd_pci.o: /usr/include/sys/time.h /usr/include/sys/unistd.h
ccd_pci.o: /usr/include/string.h /usr/include/fcntl.h
ccd_pci.o: /usr/include/sys/fcntl.h /usr/include/signal.h
ccd_pci.o: /usr/include/sys/signal.h /usr/include/errno.h
ccd_pci.o: /usr/include/sys/errno.h
ccd_pci.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_pci.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_pci.o: /space/home/dev/src/ccd/include/ccd_pci.h
ccd_text.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_text.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_text.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_text.o: /usr/include/string.h /usr/include/fcntl.h
ccd_text.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ccd_text.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_text.o: /usr/include/sys/time.h /usr/include/sys/time.h
ccd_text.o: /usr/include/sys/fcntl.h /usr/include/signal.h
ccd_text.o: /usr/include/sys/signal.h /usr/include/sys/unistd.h
ccd_text.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_text.o: /usr/include/unistd.h
ccd_text.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_text.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_text.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_text.o: /space/home/dev/src/ccd/include/ccd_pci.h
ccd_text.o: /space/home/dev/src/ccd/include/ccd_text.h
ccd_global.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_global.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_global.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_global.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_global.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ccd_global.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_global.o: /usr/include/sys/time.h /usr/include/sys/time.h
ccd_global.o: /usr/include/string.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_pci.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_text.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_exposure.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_temperature.h
ccd_global.o: /space/home/dev/src/ccd/include/ccd_setup.h
ccd_dsp.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_dsp.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_dsp.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_dsp.o: /usr/include/stdlib.h /usr/include/fcntl.h
ccd_dsp.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ccd_dsp.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_dsp.o: /usr/include/sys/time.h /usr/include/sys/time.h
ccd_dsp.o: /usr/include/sys/fcntl.h /usr/include/signal.h
ccd_dsp.o: /usr/include/sys/signal.h /usr/include/sys/unistd.h
ccd_dsp.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_dsp.o: /usr/include/string.h /usr/include/unistd.h
ccd_dsp.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_dsp.o: /space/home/dev/src/ccd/include/ccd_pci.h
ccd_dsp.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_dsp.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_dsp.o: /space/home/dev/src/cfitsio/fitsio.h /usr/include/stddef.h
ccd_dsp.o: /space/home/dev/src/cfitsio/longnam.h
ccd_temperature.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_temperature.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_temperature.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_temperature.o: /usr/include/string.h /usr/include/math.h
ccd_temperature.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_temperature.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_temperature.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_temperature.o: /space/home/dev/src/ccd/include/ccd_temperature.h
ccd_setup.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_setup.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_setup.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_setup.o: /usr/include/string.h /usr/include/fcntl.h
ccd_setup.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ccd_setup.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_setup.o: /usr/include/sys/time.h /usr/include/sys/time.h
ccd_setup.o: /usr/include/sys/fcntl.h /usr/include/signal.h
ccd_setup.o: /usr/include/sys/signal.h /usr/include/sys/unistd.h
ccd_setup.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_setup.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_setup.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_setup.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_setup.o: /space/home/dev/src/ccd/include/ccd_temperature.h
ccd_setup.o: /space/home/dev/src/ccd/include/ccd_setup.h
ccd_exposure.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ccd_exposure.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ccd_exposure.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ccd_exposure.o: /usr/include/string.h /usr/include/fcntl.h
ccd_exposure.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ccd_exposure.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ccd_exposure.o: /usr/include/sys/time.h /usr/include/sys/time.h
ccd_exposure.o: /usr/include/sys/fcntl.h /usr/include/signal.h
ccd_exposure.o: /usr/include/sys/signal.h /usr/include/sys/unistd.h
ccd_exposure.o: /usr/include/errno.h /usr/include/sys/errno.h
ccd_exposure.o: /space/home/dev/src/ccd/include/ccd_exposure.h
ccd_exposure.o: /space/home/dev/src/ccd/include/ccd_global.h
ccd_exposure.o: /space/home/dev/src/ccd/include/ccd_interface.h
ccd_exposure.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ccd_exposure.o: /space/home/dev/src/ccd/include/ccd_setup.h
ngat_ccd_CCDLibrary.o: /usr/include/stdio.h /usr/include/sys/feature_tests.h
ngat_ccd_CCDLibrary.o: /usr/include/sys/isa_defs.h /usr/include/sys/va_list.h
ngat_ccd_CCDLibrary.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
ngat_ccd_CCDLibrary.o: /usr/java1.2/include/jni.h /usr/include/stdarg.h
ngat_ccd_CCDLibrary.o: /usr/java1.2/include/solaris/jni_md.h
ngat_ccd_CCDLibrary.o: /usr/include/sys/types.h /usr/include/sys/machtypes.h
ngat_ccd_CCDLibrary.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
ngat_ccd_CCDLibrary.o: /usr/include/sys/time.h /usr/include/sys/time.h
ngat_ccd_CCDLibrary.o: /usr/include/limits.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_global.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_interface.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_dsp.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_exposure.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_setup.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_temperature.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ccd_text.h
ngat_ccd_CCDLibrary.o: /space/home/dev/src/ccd/include/ngat_ccd_CCDLibrary.h
