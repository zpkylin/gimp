## Process this file with automake to produce Makefile.in

CC = $(CCC)

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = parsley 

parsley_SOURCES = \
	parsley.c		\
	parsley.h

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)			\
	-I/software/thalia/h            \
	-I/software/thalia/h/NewWay

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
        -L/software/thalia/lib/$(PLATFORM) -L/software/thalia/lib -lhu -lpts -lbuf -lclass -ldm \
	-ltxt -lio -lutil /usr/lib32/libimage.a \
	$(X_LIBS)						\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la			\
	$(top_builddir)/libgimp/libgimp.la

parsley_DEPENDENCIES = $(DEPS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
