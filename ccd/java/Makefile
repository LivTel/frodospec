# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/java/Makefile,v 0.6 1999-06-24 12:38:57 dev Exp $ 
include ../../Makefile.common
include ../Makefile.common

INCDIR = ../include
DOCSDIR = ../javadocs
DOCFLAGS = -version -author -private

SRCS = CCDLibrary.java CCDLibraryDouble.java CCDLibraryFormatException.java ExposureThread.java Test.java SetupThread.java AbortThread.java ReadOutThread.java
OBJS = $(SRCS:%.java=%.class)
DOCS = $(SRCS:%.java=$(DOCSDIR)/%.html)

top: $(OBJS) $(DOCS)

CCDLibrary.class: CCDLibrary.java
	$(JAVAC) $(JAVAC_OPTIONS) CCDLibrary.java
	$(JAVAH) -jni CCDLibrary
	$(MV) CCDLibrary.h $(INCDIR)

CCDLibraryFormatException.class: CCDLibraryFormatException.java
	$(JAVAC) $(JAVAC_OPTIONS) CCDLibraryFormatException.java

CCDLibraryDouble.class: CCDLibraryDouble.java
	$(JAVAC) $(JAVAC_OPTIONS) CCDLibraryDouble.java

Test.class: Test.java CCDLibrary.class ExposureThread.class AbortThread.class SetupThread.class ReadOutThread.class
	$(JAVAC) $(JAVAC_OPTIONS) Test.java

ExposureThread.class: ExposureThread.java CCDLibrary.class
	$(JAVAC) $(JAVAC_OPTIONS) ExposureThread.java

AbortThread.class: AbortThread.java CCDLibrary.class
	$(JAVAC) $(JAVAC_OPTIONS) AbortThread.java

SetupThread.class: SetupThread.java CCDLibrary.class
	$(JAVAC) $(JAVAC_OPTIONS) SetupThread.java

ReadOutThread.class: ReadOutThread.java CCDLibrary.class
	$(JAVAC) $(JAVAC_OPTIONS) ReadOutThread.java

$(DOCSDIR)/CCDLibrary.html $(DOCSDIR)/CCDLibraryDouble.html $(DOCSDIR)/SetupThread.html \
$(DOCSDIR)/ExposureThread.html $(DOCSDIR)/ReadOutThread.html: CCDLibrary.java CCDLibraryDouble.java \
SetupThread.java ExposureThread.java ReadOutThread.java
	$(JAVADOC) -d $(DOCSDIR) $(DOCFLAGS) *.java

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)

checkin:
	$(CI) $(SRCS)
