# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/java/Makefile,v 0.3 1999-03-03 14:46:56 dev Exp $ 


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
	rm -f $(OBJS) *.bak *.BAK *~ #*#

checkout:
	$(CO)   -l $(SRCS)
