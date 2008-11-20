# Makefile
# $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/Makefile,v 1.1 2008-11-20 11:34:45 cjm Exp $
include ../../../Makefile.common
include ../../../../Makefile.common

MAKE 		= make
PACKAGEDIR	= ngat/frodospec/
BINDIR 		= $(FRODOSPEC_BIN_HOME)/java/$(PACKAGEDIR)
CONFIG_BINDIR 	= $(FRODOSPEC_BIN_HOME)/java
DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/javadocs/$(PACKAGEDIR)
DOCFLAGS 	= -version -author -private
SRCS 		= $(MAIN_SRCS) $(IMPL_SRCS)
MAIN_SRCS 	= FrodoSpecConstants.java FrodoSpecStatus.java \
		FrodoSpecTCPServer.java FrodoSpecTCPServerConnectionThread.java FrodoSpecREBOOTQuitThread.java \
		FrodoSpec.java 
IMPL_SRCS = $(BASE_IMPL_SRCS) $(CALIBRATE_IMPL_SRCS) $(EXPOSE_IMPL_SRCS) $(INTERRUPT_IMPL_SRCS) $(SETUP_IMPL_SRCS)
BASE_IMPL_SRCS	=JMSCommandImplementation.java CommandImplementation.java UnknownCommandImplementation.java \
		HardwareImplementation.java FITSImplementation.java FocusStage.java Plc.java LampController.java
CALIBRATE_IMPL_SRCS = CALIBRATEImplementation.java ARCImplementation.java BIASImplementation.java \
	DARKImplementation.java DAY_CALIBRATEImplementation.java 
EXPOSE_IMPL_SRCS= EXPOSEImplementation.java MULTRUNImplementation.java
INTERRUPT_IMPL_SRCS = INTERRUPTImplementation.java GET_STATUSImplementation.java REBOOTImplementation.java 
SETUP_IMPL_SRCS	= SETUPImplementation.java CONFIGImplementation.java
OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)
DOCS 		= $(SRCS:%.java=$(DOCSDIR)/%.html)

CONFIG_SRCS	= frodospec1.frodospec.properties frodospec1.frodospec.net.properties frodospec1.fits.properties \
		ltobs9.frodospec.properties ltobs9.frodospec.net.properties ltobs9.fits.properties \
		estar6.frodospec.properties estar6.frodospec.net.properties estar6.fits.properties 
CONFIG_OBJS	= $(CONFIG_SRCS:%.properties=$(CONFIG_BINDIR)/%.properties)

DIRS 		= ccd newmark test
#df1
 
top: $(OBJS) config
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

$(BINDIR)/%.class:%.java
	$(JAVAC) -sourcepath .:/home/dev/src/frodospec/java -classpath \
		$(FRODOSPEC_BIN_HOME)/java:$(CLASSPATH) $(JAVAC_OPTIONS) -d $(FRODOSPEC_BIN_HOME)/java $<
#-sourcepath .:/home/dev/src:/home/dev/src/frodospec/java

config: $(CONFIG_OBJS)

$(CONFIG_BINDIR)/%.properties: %.properties
	$(CP) $< $@

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)
	-@for i in $(DIRS); \
	do \
		(echo checkin in $$i...; cd $$i; $(MAKE) checkin; $(CI) $(CI_OPTIONS) Makefile); \
	done;

checkout:
	$(CO) $(CO_OPTIONS) $(SRCS)
	@for i in $(DIRS); \
	do \
		(echo checkout in $$i...; cd $$i; $(CO) $(CO_OPTIONS) Makefile; $(MAKE) checkout); \
	done;

depend:
	echo no depend in ngat directory.

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

backup: tidy checkin
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; cd $$i; $(MAKE) backup); \
	done;

#
# $Log: not supported by cvs2svn $
#
