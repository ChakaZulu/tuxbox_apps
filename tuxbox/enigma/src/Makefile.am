INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/enigma/src/core/gui \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src \
	-I$(top_srcdir)/enigma/src/core/base \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/enigma/src/include \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/include

bin_PROGRAMS = enigma

enigma_SOURCES = \
	bselect.cpp enigma.cpp enigma_dyn.cpp enigma_event.cpp enigma_lcd.cpp \
	enigma_main.cpp enigma_plugins.cpp enigma_setup.cpp enigma_xmlrpc.cpp scan.cpp \
	setupnetwork.cpp setupvideo.cpp showbnversion.cpp sselect.cpp streaminfo.cpp \
	enigma_mainmenu.cpp satconfig.cpp epgwindow.cpp

enigma_LDADD = \
	$(top_builddir)/enigma/src/core/driver/libdriver.la \
	$(top_builddir)/enigma/src/core/base/libbase.la \
	$(top_builddir)/enigma/src/core/dvb/libdvb.la \
	$(top_builddir)/enigma/src/core/dvb/lowlevel/liblowlevel.la \
	$(top_builddir)/enigma/src/core/gui/libgui.la \
	$(top_builddir)/enigma/src/core/gdi/libgdi.la \
	$(top_builddir)/enigma/src/core/system/libsystem.la \
	$(top_builddir)/enigma/src/core/xml/libxml.la \
	$(top_builddir)/edvb/miniqt/libminiqt.la \
	$(LIBSIGC_LIBS) @INTLLIBS@ \
	-ldl -lpthread

enigma_LDFLAGS = -rpath $(LIBDIR)/enigma

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

$(top_builddir)/enigma/src/core/base/libbase.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/base

$(top_builddir)/enigma/src/core/driver/libdriver.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/driver

$(top_builddir)/enigma/src/core/dvb/libdvb.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/dvb

$(top_builddir)/enigma/src/core/dvb/lowlevel/liblowlevel.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/dvb/lowlevel

$(top_builddir)/enigma/src/core/gdi/libgdi.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/gdi

$(top_builddir)/enigma/src/core/gui/libgui.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/gui

$(top_builddir)/enigma/src/core/system/libsystem.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/system

$(top_builddir)/enigma/src/core/xml/libxml.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/xml

$(top_builddir)/edvb/miniqt/libminiqt.la:
	$(MAKE) -C $(top_builddir)/edvb/miniqt

