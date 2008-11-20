# Makefile
# $Header: /home/cjm/cvs/frodospec/java/Makefile,v 1.1 2008-11-20 11:34:46 cjm Exp $
include ../Makefile.common
include ../../Makefile.common

MAKE 		= make
DIRS 		= ngat
JAR_FILE       	= ngat_frodospec.jar
LIBDIR		= $(FRODOSPEC_BIN_HOME)/java/
JARLIBDIR	= $(LT_BIN_HOME)/javalib

DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/javadocs/
DOCFLAGS 	= -version -author -private
JAVASRCS 	= ngat/frodospec/*.java ngat/frodospec/ccd/*.java \
		ngat/frodospec/newmark/*.java ngat/frodospec/newmark/test/*.java 
#ngat/frodospec/test/*.java  ngat/frodospec/ccd/test/*.java
#ngat/frodospec/df1/*.java ngat/frodospec/df1/test/*.java
JAVAPKGS	= ngat.frodospec ngat.frodospec.test ngat.frodospec.ccd ngat.frodospec.ccd.test \
		ngat.frodospec.newmark ngat.frodospec.newmark.test
#ngat.frodospec.df1 ngat.frodospec.df1.test
JAVADOCS 	= $(JAVASRCS:%.java=$(DOCSDIR)/%.html)

top: dirs jar javadocs

dirs:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

jar: 
	( cd $(LIBDIR); $(JAR) $(JAR_OPTIONS) $(JARLIBDIR)/$(JAR_FILE) ngat )

#
# Do all javadocs at top level, to ensure javadoc creates package links correctly
#
javadocs: $(JAVADOCS)

$(JAVADOCS): $(JAVASRCS)
	$(JAVADOC) -sourcepath /home/dev/src:/home/dev/src/frodospec/java -d $(DOCSDIR) $(DOCFLAGS) \
		-link http://java.sun.com/j2se/1.4.2/docs/api/ $(JAVAPKGS)

checkin:
	-@for i in $(DIRS); \
	do \
		(echo checkin in $$i...; cd $$i; $(MAKE) checkin; $(CI) $(CI_OPTIONS) Makefile); \
	done;

checkout:
	@for i in $(DIRS); \
	do \
		(echo checkout in $$i...; cd $$i; $(CO) $(CO_OPTIONS) Makefile; $(MAKE) checkout); \
	done;

depend:
	echo no depend in ngat directory.

clean:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-$(RM) $(RM_OPTIONS) $(JARLIBDIR)/$(JAR_FILE)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;
	$(RM) $(RM_OPTIONS) $(JAVADOCS)
	$(RM) $(RM_OPTIONS) -r $(DOCSDIR)/*

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;

backup: checkin
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; cd $$i; $(MAKE) backup); \
	done;

#
# $Log: not supported by cvs2svn $
#
