INCLUDES = \
	-I$(DRIVER)/lcd \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/gui

noinst_LTLIBRARIES = libgdi.la

libgdi_la_SOURCES = \
	epng.cpp fb.cpp font.cpp gfbdc.cpp glcddc.cpp gpixmap.cpp grc.cpp \
	lcd.cpp

libgdi_la_LIBADD = $(LIBFREETYPE_LIBS) $(LIBPNG_LIBS)

libgdi_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libgdi.la $(LIBDIR)/enigma

