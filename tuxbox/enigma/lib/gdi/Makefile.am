INCLUDES = \
	-I$(DRIVER)/lcd \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/system

lib_LTLIBRARIES = libgdi.la

libgdi_la_SOURCES = \
	epng.cpp fb.cpp font.cpp gfbdc.cpp glcddc.cpp gpixmap.cpp grc.cpp \
	lcd.cpp

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS)

