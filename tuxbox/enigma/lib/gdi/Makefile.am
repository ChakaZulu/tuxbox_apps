INCLUDES = \
	-I$(DRIVER)/lcd \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/base \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/gui \
	-I$(top_srcdir)/enigma/src/include

lib_LTLIBRARIES = libenigma_gdi.la

libenigma_gdi_la_SOURCES = \
	epng.cpp fb.cpp font.cpp gfbdc.cpp glcddc.cpp gpixmap.cpp grc.cpp \
	lcd.cpp

libenigma_gdi_la_LIBADD = $(LIBFREETYPE_LIBS) $(LIBPNG_LIBS)

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

