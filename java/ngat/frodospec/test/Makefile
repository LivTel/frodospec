# Makefile
# $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/Makefile,v 1.4 2014-08-28 17:02:42 cjm Exp $ 
include ../../../../../Makefile.common
include	../../../../Makefile.common

PACKAGEDIR	= ngat/frodospec/test
BINDIR 		= $(FRODOSPEC_BIN_HOME)/java/$(PACKAGEDIR)
DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/javadocs/$(PACKAGEDIR)
DOCFLAGS 	= -version -author -private

SRCS 		= SicfTCPClientConnectionThread.java SicfTCPServerConnectionThread.java SicfTCPServer.java \
		 SendConfigCommand.java SendMultrunCommand.java SendBiasCommand.java SendDarkCommand.java \
		 TestPlc.java TestLampController.java
OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)
DOCS 		= $(SRCS:%.java=$(DOCSDIR)/%.html)

top: $(OBJS)

$(BINDIR)/%.class:%.java
	$(JAVAC) -source 1.4 -target 1.4 -sourcepath .:/home/dev/src:/home/dev/src/supircam/java \
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
# Revision 1.3  2009/09/17 09:56:27  cjm
# Added SendBiasCommand.java SendDarkCommand.java.
#
# Revision 1.2  2009/08/20 11:22:58  cjm
# Added TestLampController.java.
#
# Revision 1.1  2008/11/20 11:34:44  cjm
# Initial revision
#
#
