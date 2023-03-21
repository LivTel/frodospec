# $Header: /home/cjm/cvs/frodospec/latex/Makefile,v 1.1 2023-03-21 14:49:04 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

DOCSDIR 	= $(FRODOSPEC_DOC_HOME)/latex

TEXSRCS 	= frodospec_fault_diagnosis.tex
FIGSRCS 	= 
PNGSRCS		= frodospec_icsgui_connection_refused.png

EPS_SRCS	= frodospec_basic_fault_diagnosis1.eps frodospec_instrument_status_fail1.eps \
		frodospec_instrument_status_fail2.eps frodospec_uncontactable1.eps frodospec_plc_problem1.eps

DVIOBJS		= $(TEXSRCS:%.tex=%.dvi)
PSOBJS		= $(TEXSRCS:%.tex=$(DOCSDIR)/%.ps)

EPSOBJS		= $(FIGSRCS:%.fig=%.eps) $(PNGSRCS:%.png=%.eps) $(EPS_SRCS)

TXTSRCS		= 
TXTOBJS		= $(TXTSRCS:%.txt=$(DOCSDIR)/%.txt)

DIRS 		= design

top: $(PSOBJS) $(TXTOBJS)

$(DOCSDIR)/%.ps: %.dvi
	$(DVIPS) $< -o
	$(MV) $*.ps $(DOCSDIR)

$(DVIOBJS): $(EPSOBJS)

%.dvi: %.tex
	$(LATEX) $<
	$(LATEX) $<
	$(LATEX) $<

%.eps: %.jpg
	$(CONVERT) $< $@

%.eps: %.png
	$(CONVERT) $< $@

%.eps: %.fig
	$(FIG2DEV) $(FIG2DEV_OPTIONS) $< $@

$(DOCSDIR)/%.txt: %.txt
	$(CP) $< $(DOCSDIR)
	$(CHMOD) ug+w $@

depend:
	echo "NO depend rule for latex files."

clean:
	$(RM) $(RM_OPTIONS) $(DVIOBJS) $(PSOBJS) $(EPSOBJS) $(TXTOBJS) $(LATEX_TIDY_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo cleaning in $$i...; cd $$i; $(MAKE) clean); \
	done;

tidy:
	-$(RM) $(RM_OPTIONS) $(LATEX_TIDY_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidying in $$i...; cd $$i; $(MAKE) tidy); \
	done;

backup: tidy
	-$(RM) $(RM_OPTIONS) $(DVIOBJS) $(EPSOBJS)
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; cd $$i; $(MAKE) backup); \
	done;

checkin:
	-$(CI) $(CI_OPTIONS) $(TEXSRCS) $(FIGSRCS) $(TXTSRCS)
	@for i in $(DIRS); \
	do \
		(echo checking in in $$i...; cd $$i; $(MAKE) checkin); \
	done;

checkout:
	-$(CO) $(CO_OPTIONS) $(TEXSRCS) $(FIGSRCS) $(TXTSRCS)
	@for i in $(DIRS); \
	do \
		(echo checking out in $$i...; cd $$i; $(MAKE) checkout); \
	done;

#
# $Log: not supported by cvs2svn $
#

