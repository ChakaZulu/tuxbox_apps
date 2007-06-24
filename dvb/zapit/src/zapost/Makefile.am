INCLUDES = \
	-I$(top_srcdir)/include \
	@XML_CFLAGS@

noinst_LTLIBRARIES = libzapost.la

if BOXTYPE_DREAMBOX
libzapost_la_SOURCES = audio.cpp dmx.cpp frontend.cpp video.cpp
else
libzapost_la_SOURCES = audio.cpp aviaext.cpp dmx.cpp frontend.cpp video.cpp
endif

AM_CPPFLAGS = -DDEBUG_SEC_TIMING
