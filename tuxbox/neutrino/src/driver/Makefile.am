INCLUDES = \
	-I@DRIVER@/include \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I$(top_srcdir)/daemons/timerd \
	-I$(top_srcdir)/libs/libcontroldclient \
	-I$(top_srcdir)/libs/liblcddclient \
	-I$(top_srcdir)/libs/libsectionsdclient \
	-I$(top_srcdir)/libs/libtimerdclient \
	-I$(top_srcdir)/gui

noinst_LIBRARIES = libneutrino_driver.a

libneutrino_driver_a_SOURCES = fontrenderer.cpp framebuffer.cpp rcinput.cpp vcrcontrol.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

