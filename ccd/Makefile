# Makefile -*- mode: Fundamental;-*-
# $Header: /home/cjm/cvs/frodospec/ccd/Makefile,v 1.1 2003-11-14 15:32:30 cjm Exp $ 

include ../Makefile.common

DIRS = c test java latex misc

top:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

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
	@for i in c misc; \
	do \
		(echo depend in $$i...; cd $$i; $(MAKE) depend);\
	done;

clean:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;

backup: checkin
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; cd $$i; $(MAKE) backup); \
	done;
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	$(RM) $(RM_OPTIONS) */*.fits
	$(RM) $(RM_OPTIONS) */core
	$(RM) $(RM_OPTIONS) misc/dsp_command_num
	tar cvf $(BACKUP_DIR)/ccd.tar .
	compress $(BACKUP_DIR)/ccd.tar


# $Log: not supported by cvs2svn $
#
