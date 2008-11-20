# Makefile
# $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/Makefile,v 1.1 2008-11-20 11:34:44 cjm Exp $ 
include ../../../../../Makefile.common
include	../../../../Makefile.common

PACKAGEDIR	= ngat/frodospec/test
BINDIR 		= $(FRODOSPEC_BIN_HOME)/java/$(PACKAGEDIR)
DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/javadocs/$(PACKAGEDIR)
DOCFLAGS 	= -version -author -private

SRCS 		= SicfTCPClientConnectionThread.java SicfTCPServerConnectionThread.java SicfTCPServer.java \
		 SendConfigCommand.java SendMultrunCommand.java TestPlc.java
OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)
DOCS 		= $(SRCS:%.java=$(DOCSDIR)/%.html)

top: $(OBJS)

$(BINDIR)/%.class:%.java
	$(JAVAC) -sourcepath .:/home/dev/src:/home/dev/src/supircam/java \
		-classpath $(FRODOSPEC_BIN_HOME)/java:$(CLASSPATH) $(JAVAC_OPTIONS) -d $(FRODOSPEC_BIN_HOME)/java $<

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)

clean: tidy
	-$(RM) $(RM_OPTIONS) $(OBJS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

backup: tidy
#
# $Log: not supported by cvs2svn $
#
