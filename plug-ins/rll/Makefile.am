## Process this file with automake to produce Makefile.in

CC = $(CCC) 


pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = rll 

rll_SOURCES = \
	rll.c   

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)			\
	-I/software/thalia/h

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
    	-L/software/thalia/lib/$(PLATFORM) -L/software/thalia/lib32  -lbuf -lclass -ltxt -lio -limage -lutil -lm \
	$(X_LIBS)				\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la

rll_DEPENDENCIES = $(DEPS)

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
