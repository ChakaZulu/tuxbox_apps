SUBDIRS = pictureviewer

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	-I$(top_srcdir)/lib

noinst_LIBRARIES = libneutrino_driver.a

libneutrino_driver_a_SOURCES = \
	encoding.cpp \
	fontrenderer.cpp \
	framebuffer.cpp \
	fb_window.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	mp3play.cpp \
	lcdd.cpp \
	bigclock.cpp \
	newclock.cpp \
	pig.cpp \
	stream2file.cpp \
	screen_max.cpp

AM_CPPFLAGS = -DSKIP_CA_STATUS
