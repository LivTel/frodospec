# Makefile
# $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ccd/Makefile,v 1.2 2009-03-05 16:18:22 cjm Exp $
include ../../../../../Makefile.common
include	../../../../Makefile.common

PACKAGEDIR	= ngat/frodospec/ccd
BINDIR 		= $(FRODOSPEC_BIN_HOME)/java/$(PACKAGEDIR)
DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/javadocs/$(PACKAGEDIR)
DOCFLAGS 	= -version -author -private
SRCS 		= CCDLibraryNativeException.java CCDLibraryFormatException.java CCDLibrarySetupWindow.java \
		CCDLibrary.java
OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)
DOCS 		= $(SRCS:%.java=$(DOCSDIR)/%.html)

DIRS 		= test

top: $(OBJS)
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

$(BINDIR)/%.class:%.java
	$(JAVAC) -sourcepath .:/home/dev/src:/home/dev/src/frodospec/java \
		-classpath $(FRODOSPEC_BIN_HOME)/java:$(CLASSPATH) $(JAVAC_OPTIONS) -d $(FRODOSPEC_BIN_HOME)/java $<

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)
	@for i in $(DIRS); \
	do \
		(echo checkout in $$i...; cd $$i; $(CO) $(CO_OPTIONS) Makefile; $(MAKE) checkout); \
	done;

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)
	-@for i in $(DIRS); \
	do \
		(echo checkin in $$i...; cd $$i; $(MAKE) checkin; $(CI) $(CI_OPTIONS) Makefile); \
	done;

clean: tidy
	-$(RM) $(RM_OPTIONS) $(OBJS)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;

backup: tidy
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; cd $$i; $(MAKE) backup); \
	done;
#
# $Log: not supported by cvs2svn $
# Revision 1.1  2008/11/20 11:34:34  cjm
# Initial revision
#
#
