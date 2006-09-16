
INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/daemons/nhttpd \
	-I$(top_srcdir)/daemons/nhttpd/yhttpd_core \
	-I$(top_srcdir)/daemons/nhttpd/yhttpd_mods \
	@FREETYPE_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@

noinst_LIBRARIES = libnhttpd_tuxboxapi.a

libnhttpd_tuxboxapi_a_SOURCES = \
	neutrinoapi.cpp neutrinoyparser.cpp lcdapi.cpp controlapi.cpp
	