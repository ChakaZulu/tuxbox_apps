SUBDIRS = icons

INCLUDES = -I@DRIVER@/include \
	   -I@DRIVER@/lcd \
	   @LCDDISPLAY_CFLAGS@ \
	   @FREETYPE_CFLAGS@ \
	   -I$(top_srcdir)/libs/liblcddclient

bin_PROGRAMS = lcdd

lcdd_SOURCES = lcdd.cpp lcdpainter.cpp bigclock.cpp newclock.cpp

lcdd_LDADD = \
	$(top_builddir)/libs/liblcddclient/liblcddclient.la \
	@CONFIGFILE_LIBS@ \
	@LCDDISPLAY_LIBS@ \
	@FREETYPE_LIBS@ \
	-lpthread

