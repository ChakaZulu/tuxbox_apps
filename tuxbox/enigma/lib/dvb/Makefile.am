SUBDIRS = lowlevel

INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src \
	-I$(top_srcdir)/enigma/src/core/base \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/include

lib_LTLIBRARIES = libenigma_dvb.la

libenigma_dvb_la_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp epgcache.cpp \
	esection.cpp frontend.cpp si.cpp record.cpp \
	dvbwidgets.cpp

AM_CPPFLAGS = -DPROFILE -DDBOX 
AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

