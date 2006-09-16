

INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/daemons/nhttpd \
	-I$(top_srcdir)/daemons/nhttpd/yhttpd_core \
	@FREETYPE_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@


noinst_LIBRARIES = libyhttpdmods.a

libyhttpdmods_a_SOURCES = \
	mod_yparser.cpp mod_testhook.cpp mod_auth.cpp

