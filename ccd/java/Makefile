# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/java/Makefile,v 0.9 1999-08-23 14:04:16 cjm Exp $ 
include ../../Makefile.common
include ../Makefile.common

INCDIR = $(CCD_SRC_HOME)/include
BINDIR = $(CCD_BIN_HOME)/java
DOCSDIR = $(CCD_DOC_HOME)/javadocs
DOCFLAGS = -version -author -private

SRCS = CCDLibrary.java CCDLibraryDouble.java CCDLibraryFormatException.java ExposureThread.java Test.java SetupThread.java AbortThread.java ReadOutThread.java
OBJS = $(SRCS:%.java=$(BINDIR)/%.class)
DOCS = $(SRCS:%.java=$(DOCSDIR)/%.html)

top: $(OBJS) docs

$(BINDIR)/CCDLibrary.class: CCDLibrary.java
	$(JAVAC) $(JAVAC_OPTIONS) -d $(BINDIR) CCDLibrary.java
	$(JAVAH) -jni -d $(INCDIR) CCDLibrary

$(BINDIR)/%.class:%.java CCDLibrary.class
	$(JAVAC) $(JAVAC_OPTIONS) -d $(BINDIR) $<

Test.class: Test.java CCDLibrary.class ExposureThread.class AbortThread.class SetupThread.class ReadOutThread.class
	$(JAVAC) $(JAVAC_OPTIONS) -d $(BINDIR) Test.java

docs: $(DOCS)

$(DOCS): $(SRCS)
	$(JAVADOC) -d $(DOCSDIR) $(DOCFLAGS) $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)

checkin:
	$(CI) $(SRCS)
#
# $Log: not supported by cvs2svn $
# Revision 0.8  1999/08/23 13:54:36  cjm
# Added Log comment.
#
#
