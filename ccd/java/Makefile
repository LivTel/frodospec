# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/java/Makefile,v 0.2 1999-02-24 12:07:44 dev Exp $ 


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

Test.class: Test.java CCDLibrary.class ExposureThread.class AbortThread.class SetupThread.class
	javac Test.java

ExposureThread.class: ExposureThread.java CCDLibrary.class
	javac ExposureThread.java

AbortThread.class: AbortThread.java CCDLibrary.class
	javac AbortThread.java

SetupThread.class: SetupThread.java CCDLibrary.class
	javac SetupThread.java

$(DOCSDIR)/CCDLibrary.html $(DOCSDIR)/CCDLibraryDouble.html $(DOCSDIR)/SetupThread.html \
$(DOCSDIR)/ExposureThread.html: CCDLibrary.java CCDLibraryDouble.java SetupThread.java ExposureThread.java
	javadoc -d $(DOCSDIR) $(DOCFLAGS) *.java

clean:
	rm -f $(OBJS) *.bak *.BAK *~ #*#

