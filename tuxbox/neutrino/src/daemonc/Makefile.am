INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I@DRIVER@/include

noinst_LIBRARIES = libneutrino_daemonc.a

libneutrino_daemonc_a_SOURCES = remotecontrol.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

