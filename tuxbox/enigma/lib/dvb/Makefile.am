SUBDIRS = lowlevel

INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/system

noinst_LIBRARIES = libdvb.a

libdvb_a_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp edvb.moc.cpp epgcache.cpp \
	epgcache.moc.cpp esection.cpp esection.moc.cpp frontend.cpp \
	frontend.moc.cpp si.cpp si.moc.cpp

AM_CPPFLAGS = -DPROFILE

%.moc.cpp: %.h
	$(MOC) $< -o $@

clean-local:
	-rm *.moc.cpp
