INCLUDES = \
	-I$(top_srcdir)/include \
	@XML_CFLAGS@

noinst_LTLIBRARIES = libzapost.la

libzapost_la_SOURCES = audio.cpp dmx.cpp video.cpp
if BOXTYPE_DBOX2
libzapost_la_SOURCES += aviaext.cpp
endif
if BOXTYPE_TRIPLE
libzapost_la_SOURCES += frontend-td.cpp
else
libzapost_la_SOURCES += frontend.cpp
endif

# AM_CPPFLAGS = -DDEBUG_SEC_TIMING
