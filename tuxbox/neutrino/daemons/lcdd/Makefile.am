SUBDIRS = icons

INCLUDES = -I@DRIVER@/include \
	   -I@DRIVER@/lcd \
	   -I$(top_srcdir)/liblcddisplay \
	   -I$(top_srcdir)/libs/liblcddclient

bin_PROGRAMS = lcdd

lcdd_SOURCES = lcdd.cpp lcdpainter.cpp bigclock.cpp newclock.cpp

lcdd_LDADD = \
	$(top_builddir)/libs/liblcddclient/liblcddclient.la \
	$(top_builddir)/libconfigfile/libconfigfile.la \
	$(top_builddir)/liblcddisplay/liblcddisplay.la \
	-lpthread

#AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS)

