SUBDIRS = pictureviewer audiodec

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
	ringbuffer.c \
	encoding.cpp \
	fontrenderer.cpp \
	framebuffer.cpp \
	fb_window.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	audioplay.cpp \
	lcdd.cpp \
	bigclock.cpp \
	newclock.cpp \
	pig.cpp \
	stream2file.cpp \
	screen_max.cpp \
	netfile.cpp \
	slotbuffer.c

AM_CPPFLAGS = -DSKIP_CA_STATUS
