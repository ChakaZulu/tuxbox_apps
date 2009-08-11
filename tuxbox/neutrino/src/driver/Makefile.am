SUBDIRS = pictureviewer

if ENABLE_AUDIOPLAYER
SUBDIRS += audiodec
endif

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@MPEGTOOLS_CFLAGS@ \
	-I$(top_srcdir)/lib

noinst_LIBRARIES = libneutrino_driver.a

if ENABLE_INTERNETRADIO
noinst_LIBRARIES += libneutrino_driver_netfile.a
endif

libneutrino_driver_a_SOURCES = \
	ringbuffer.c \
	encoding.cpp \
	fontrenderer.cpp \
	framebuffer.cpp \
	fb_window.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	lcdd.cpp \
	bigclock.cpp \
	newclock.cpp \
	pig.cpp \
	genpsi.c \
	stream2file.cpp \
	screen_max.cpp \
	slotbuffer.c \
	file.cpp \
	shutdown_count.cpp \
	BitrateCalculator.cpp

if ENABLE_RADIOTEXT
libneutrino_driver_a_SOURCES += \
	radiotext.cpp \
	radiotools.cpp
endif

if ENABLE_AUDIOPLAYER
libneutrino_driver_a_SOURCES += \
	audioplay.cpp \
	audiometadata.cpp \
	audiofile.cpp
endif

if ENABLE_INTERNETRADIO
libneutrino_driver_netfile_a_SOURCES = netfile.cpp
endif
