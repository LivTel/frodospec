# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/java/Makefile,v 0.4 1999-04-27 10:55:41 dev Exp $ 
include ../../Makefile.common
include ../Makefile.common

INCDIR = ../include
DOCSDIR = ../javadocs
DOCFLAGS = -version -author -private

SRCS = CCDLibrary.java CCDLibraryDouble.java ExposureThread.java Test.java SetupThread.java AbortThread.java ReadOutThread.java
OBJS = $(SRCS:%.java=%.class)
DOCS = $(SRCS:%.java=$(DOCSDIR)/%.html)

top: $(OBJS) $(DOCS)

CCDLibrary.class: CCDLibrary.java
	javac CCDLibrary.java
	javah -jni CCDLibrary
	mv CCDLibrary.h $(INCDIR)

CCDLibraryDouble.class: CCDLibraryDouble.java
	javac CCDLibraryDouble.java

Test.class: Test.java CCDLibrary.class ExposureThread.class AbortThread.class SetupThread.class ReadOutThread.class
	javac Test.java

ExposureThread.class: ExposureThread.java CCDLibrary.class
	javac ExposureThread.java

AbortThread.class: AbortThread.java CCDLibrary.class
	javac AbortThread.java

SetupThread.class: SetupThread.java CCDLibrary.class
	javac SetupThread.java

ReadOutThread.class: ReadOutThread.java CCDLibrary.class
	javac ReadOutThread.java

$(DOCSDIR)/CCDLibrary.html $(DOCSDIR)/CCDLibraryDouble.html $(DOCSDIR)/SetupThread.html \
$(DOCSDIR)/ExposureThread.html $(DOCSDIR)/ReadOutThread.html: CCDLibrary.java CCDLibraryDouble.java \
SetupThread.java ExposureThread.java ReadOutThread.java
	javadoc -d $(DOCSDIR) $(DOCFLAGS) *.java

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)

checkin:
	$(CI) $(SRCS)
