INCLUDES = -I$(top_srcdir)/include \
	   @LIBXML_CFLAGS@ \
           @XMLTREE_CFLAGS@


noinst_LTLIBRARIES = libzapost.la

libzapost_la_SOURCES = audio.cpp dmx.cpp frontend.cpp video.cpp

AM_CPPFLAGS = -pedantic -Werror
AM_CXXFLAGS = -fno-rtti -fno-exceptions
