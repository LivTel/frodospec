# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/java/Makefile,v 0.13 1999-09-13 13:54:34 cjm Exp $ 
include ../../Makefile.common
include ../Makefile.common

INCDIR = $(CCD_SRC_HOME)/include
BINDIR = $(CCD_BIN_HOME)/java
DOCSDIR = $(CCD_DOC_HOME)/javadocs
DOCFLAGS = -version -author -private

SRCS = ExposureThread.java Test.java SetupThread.java AbortThread.java ReadOutThread.java
OBJS = $(SRCS:%.java=$(BINDIR)/%.class)
DOCS = $(SRCS:%.java=$(DOCSDIR)/%.html)

top: $(OBJS) $(INCDIR)/CCDLibrary.h docs

$(INCDIR)/CCDLibrary.h:
	$(JAVAH) -jni -d $(INCDIR) ngat.ccd.CCDLibrary

$(BINDIR)/%.class:%.java
	$(JAVAC) $(JAVAC_OPTIONS) -d $(BINDIR) $<

Test.class: Test.java $(BINDIR)/ExposureThread.class $(BINDIR)/AbortThread.class $(BINDIR)/SetupThread.class $(BINDIR)/ReadOutThread.class
	$(JAVAC) $(JAVAC_OPTIONS) -d $(BINDIR) Test.java

$(BINDIR)/ExposureThread.class $(BINDIR)/AbortThread.class $(BINDIR)/SetupThread.class $(BINDIR)/ReadOutThread.class: 

docs: $(DOCS)

$(DOCS): $(SRCS)
	$(JAVADOC) -d $(DOCSDIR) $(DOCFLAGS) $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(INCDIR)/CCDLibrary.h $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)
#
# $Log: not supported by cvs2svn $
# Revision 0.12  1999/09/10 15:27:25  cjm
# Changed due to CCDLibrary moving to ngat.ccd. package.
#
# Revision 0.11  1999/09/08 10:03:09  cjm
# Changed checkin target so the return value is ignored and it uses CI_OPTIONS.
#
# Revision 0.10  1999/08/23 14:21:59  cjm
# Fixed new rule for CCDLibrary.h header file.
#
# Revision 0.9  1999/08/23 14:04:16  cjm
# Fixed docs rule
#
# Revision 0.8  1999/08/23 13:54:36  cjm
# Added Log comment.
#
#
