# Makefile
# $Header: /home/cjm/cvs/frodospec/ccd/Makefile,v 1.5 2011-01-17 11:10:19 cjm Exp $ 

include ../../Makefile.common
include ../Makefile.common

DIRS = c test 
#java latex misc

top:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; $(MAKE) -C $$i ); \
	done;

checkin:
	-@for i in $(DIRS); \
	do \
		(echo checkin in $$i...; $(MAKE) -C $$i checkin; cd $$i; $(CI) $(CI_OPTIONS) Makefile); \
	done;

checkout:
	@for i in $(DIRS); \
	do \
		(echo checkout in $$i...; cd $$i; $(CO) $(CO_OPTIONS) Makefile; $(MAKE) checkout); \
	done;

depend:
	@for i in $(DIRS); \
	do \
		(echo depend in $$i...; $(MAKE) -C $$i depend);\
	done;

clean:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; $(MAKE) -C $$i clean); \
	done;

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; $(MAKE) -C $$i tidy); \
	done;

backup: checkin
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; $(MAKE) -C $$i backup); \
	done;
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	$(RM) $(RM_OPTIONS) */*.fits
	$(RM) $(RM_OPTIONS) */core


# $Log: not supported by cvs2svn $
# Revision 1.4  2008/11/20 11:35:39  cjm
# *** empty log message ***
#
# Revision 1.3  2007/01/09 14:36:45  cjm
# Changed some 'cd's for $(MAKE) -C.
#
# Revision 1.2  2006/05/17 16:42:41  cjm
# Changed depend dirs.
#
# Revision 1.1  2003/11/14 15:32:30  cjm
# Initial revision
#
#
