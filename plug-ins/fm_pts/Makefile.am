## Process this file with automake to produce Makefile.in

CC = $(CCC)

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = fm_pts 

fm_pts_SOURCES = \
	fm_pts.c \
	bxutils-vk.c \
	main-vk.c \
	RH_FileSB.c \
	RH_FileDialog.c \
	RH_FileComboSB.c
	

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)                 \
        -I/usr/ToolTalk/include \
        -I/software/thalia/h

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	-L/software/thalia/lib/$(PLATFORM) \
	-L/software/thalia/lib\
	$(X_LIBS) \
        -lXpm -lUI -lXrh -lvk -lvkhelp -lXpm -lGLw \
	-lXm -lXmu -lGLw -lXm -lXmu -lXt -lGL -lX11 -lm  -lpts \
	-lbuf -lclass -ldm -ltxt -lio -lc -lutil -lm 

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la

fm_pts_DEPENDENCIES = $(DEPS)

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
