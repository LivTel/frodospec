# Makefile
# $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ccd/test/Makefile,v 1.1 2009-03-05 12:16:12 cjm Exp $
include ../../../../../../Makefile.common
include	../../../../../Makefile.common

PACKAGEDIR	= ngat/frodospec/ccd/test
BINDIR 		= $(FRODOSPEC_BIN_HOME)/java/$(PACKAGEDIR)
CONFIG_BINDIR 	= $(FRODOSPEC_BIN_HOME)/java
DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/javadocs/$(PACKAGEDIR)
DOCFLAGS 	= -version -author -private
SRCS 		= TestCCDRedatum.java
OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)
DOCS 		= $(SRCS:%.java=$(DOCSDIR)/%.html)
CONFIG_SRCS	= test_ccd_redatum.properties
CONFIG_OBJS	= $(CONFIG_SRCS:%.properties=$(CONFIG_BINDIR)/%.properties)
DIRS 		= 
#test

top: $(OBJS) config
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

config: $(CONFIG_OBJS)

$(CONFIG_BINDIR)/%.properties: %.properties
	$(CP) $< $@

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
#
