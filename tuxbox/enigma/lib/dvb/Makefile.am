SUBDIRS = lowlevel

INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/system

noinst_LTLIBRARIES = libdvb.la

libdvb_la_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp epgcache.cpp \
	esection.cpp frontend.cpp si.cpp

libdvb_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_CPPFLAGS = -DPROFILE -DDBOX
AM_CXXFLAGS = $(LIBSIGC_CFLAGS)

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libdvb.la $(LIBDIR)/enigma

