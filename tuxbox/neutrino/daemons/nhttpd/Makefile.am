SUBDIRS = yhttpd_core yhttpd_mods tuxboxapi web

INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/daemons/nhttpd/yhttpd_core \
	-I$(top_srcdir)/daemons/nhttpd/yhttpd_mods \
	-I$(top_srcdir)/daemons/nhttpd/tuxboxapi \
	@FREETYPE_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@

bin_PROGRAMS = nhttpd

nhttpd_SOURCES = yhttpd.cpp

nhttpd_LDADD = \
	$(top_srcdir)/daemons/nhttpd/tuxboxapi/libnhttpd_tuxboxapi.a \
	$(top_srcdir)/daemons/nhttpd/yhttpd_mods/libyhttpdmods.a \
	$(top_srcdir)/daemons/nhttpd/yhttpd_core/libyhttpd.a \
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
