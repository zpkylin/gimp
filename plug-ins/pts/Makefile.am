## Process this file with automake to produce Makefile.in

CC = $(CCC)

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = pts 

pts_SOURCES = \
	pts.c \
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
	-L/software/thalia/lib \
	$(X_LIBS) \
        -lXpm -lUI -lXrh -lvk -lvkmsg -lvkhelp -lXpm -lGLw -lSgm \
	-lXm -ltt -lGLw -lSgm -lXm -lXt -lGL -lX11 -lgen -lm  -lpts \
	-lbuf -lclass -ldm -lmi -ltxt -lio -lc -lutil -lm /usr/lib32/libimage.a

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la

pts_DEPENDENCIES = $(DEPS)

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
