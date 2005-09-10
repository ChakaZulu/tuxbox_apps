SUBDIRS = data data-alt2 nhttpd-y

INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@FREETYPE_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@

bin_PROGRAMS = nhttpd

nhttpd_SOURCES = \
	controlapi.cpp bouqueteditapi.cpp debug.cpp helper.cpp \
	nhttpd.cpp request.cpp webapi.cpp webdbox.cpp webserver.cpp lcdapi.cpp yapi.cpp

nhttpd_LDADD = \
	$(top_builddir)/lib/controldclient/libcontroldclient.la \
	$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la \
	$(top_builddir)/lib/timerdclient/libtimerdclient.la \
	@LCDDISPLAY_LIBS@ \
	@FREETYPE_LIBS@ \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@ZAPIT_LIBS@ \
	-lpthread

$(top_builddir)/lib/controldclient/libcontroldclient.la:
	$(MAKE) -C $(top_builddir)/lib/controldclient

$(top_builddir)/lib/timerdclient/libtimerdclient.la:
	$(MAKE) -C $(top_builddir)/lib/timerdclient

$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la:
	$(MAKE) -C $(top_builddir)/lib/sectionsdclient
