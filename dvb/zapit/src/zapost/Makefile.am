INCLUDES = \
	-I$(top_srcdir)/zapit \
	-I$(top_srcdir)/mczap/libevent \
	-I$(DRIVER)/include

noinst_LIBRARIES = libzapost.a

libzapost_a_SOURCES = audio.cpp dmx.cpp frontend.cpp video.cpp

AM_CPPFLAGS = -DDBOX2 -DALWAYS_SELECT_VIDEO_SOURCE

