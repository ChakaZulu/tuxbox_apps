INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I@DRIVER@/include

noinst_LIBRARIES = libneutrino_driver.a

libneutrino_driver_a_SOURCES = fontrenderer.cpp framebuffer.cpp rcinput.cpp vcrcontrol.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

