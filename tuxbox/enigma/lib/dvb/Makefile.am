SUBDIRS = lowlevel

INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/system \
	-I/home/tmbinc/dreambox/pallas2-linux_drv-20020128/include/

lib_LTLIBRARIES = libdvb.la

libdvb_la_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp edvb.moc.cpp epgcache.cpp \
	epgcache.moc.cpp esection.cpp esection.moc.cpp frontend.cpp \
	frontend.moc.cpp si.cpp si.moc.cpp

AM_CPPFLAGS = -DPROFILE -DDBOX

%.moc.cpp: %.h
	$(MOC) $< -o $@

clean-local:
	-rm *.moc.cpp
