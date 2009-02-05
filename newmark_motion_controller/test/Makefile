# $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/test/Makefile,v 1.2 2009-02-05 11:42:33 cjm Exp $

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(NEWMARK_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(NEWMARK_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(NEWMARK_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) -I$(ARCOM_ESS_INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR) $(LOG_UDP_CFLAGS)
DOCFLAGS 	= -static

SRCS 		= newmark_test_home.c newmark_test_move_absolute.c newmark_test_move_relative.c \
		newmark_test_abort_move.c newmark_test_position_get.c newmark_test_error_get.c newmark_test_move.c
OBJS 		= $(SRCS:%.c=%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: $(PROGS) docs

$(BINDIR)/%: %.o
	cc -o $@ $< -L$(LT_LIB_HOME) -l$(NEWMARK_LIBNAME) -l$(ARCOM_ESS_LIBNAME) $(TIMELIB) $(SOCKETLIB) -lm -lc

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(PROGS) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

backup: tidy
	$(RM) $(RM_OPTIONS) $(OBJS)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)

checkout:
	-$(CO) $(CO_OPTIONS) $(SRCS)

# DO NOT DELETE

