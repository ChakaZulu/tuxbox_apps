SUBDIRS = lowlevel

INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/system

noinst_LTLIBRARIES = libdvb.la

libdvb_la_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp edvb.moc.cpp epgcache.cpp \
	epgcache.moc.cpp esection.cpp esection.moc.cpp frontend.cpp \
	frontend.moc.cpp si.cpp si.moc.cpp

libdvb_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_CPPFLAGS = -DPROFILE -DDBOX

%.moc.cpp: %.h
	$(MOC) $< -o $@

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libdvb.la $(LIBDIR)/enigma

clean-local:
	-rm *.moc.cpp
