INCLUDES = \
	-I@DRIVER@/include \
	@CURL_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I$(top_srcdir)/libs/libcontroldclient \
	-I$(top_srcdir)/libs/liblcddclient \
	-I$(top_srcdir)/libs/libsectionsdclient \
	-I$(top_srcdir)/libs/libtimerdclient

noinst_LIBRARIES = libneutrino_daemonc.a

libneutrino_daemonc_a_SOURCES = remotecontrol.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

