SUBDIRS = lowlevel

INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/enigma/src

lib_LTLIBRARIES = libenigma_dvb.la

libenigma_dvb_la_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp epgcache.cpp \
	esection.cpp frontend.cpp si.cpp record.cpp \
	dvbwidgets.cpp dvbservice.cpp settings.cpp \
	dvbscan.cpp service.cpp servicedvb.cpp

AM_CPPFLAGS = -DPROFILE -DDBOX 
AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

