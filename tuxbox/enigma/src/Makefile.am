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
	enigma_mainmenu.cpp satconfig.cpp epgwindow.cpp setup_language.cpp enigma_info.cpp enigma_scan.cpp \
	setupskin.cpp

enigma_LDADD = \
	$(top_builddir)/enigma/src/core/driver/libenigma_driver.la \
	$(top_builddir)/enigma/src/core/base/libenigma_base.la \
	$(top_builddir)/enigma/src/core/dvb/libenigma_dvb.la \
	$(top_builddir)/enigma/src/core/dvb/lowlevel/libenigma_dvb_lowlevel.la \
	$(top_builddir)/enigma/src/core/gui/libenigma_gui.la \
	$(top_builddir)/enigma/src/core/gdi/libenigma_gdi.la \
	$(top_builddir)/enigma/src/core/system/libenigma_system.la \
	$(top_builddir)/enigma/src/core/xml/libenigma_xml.la \
	$(top_builddir)/edvb/miniqt/libminiqt.la \
	$(LIBSIGC_LIBS) $(INTLLIBS) \
	-ldl -lpthread

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

$(top_builddir)/enigma/src/core/base/libenigma_base.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/base

$(top_builddir)/enigma/src/core/driver/libenigma_driver.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/driver

$(top_builddir)/enigma/src/core/dvb/libenigma_dvb.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/dvb

$(top_builddir)/enigma/src/core/dvb/lowlevel/libenigma_dvb_lowlevel.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/dvb/lowlevel

$(top_builddir)/enigma/src/core/gdi/libenigma_gdi.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/gdi

$(top_builddir)/enigma/src/core/gui/libenigma_gui.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/gui

$(top_builddir)/enigma/src/core/system/libenigma_system.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/system

$(top_builddir)/enigma/src/core/xml/libenigma_xml.la:
	$(MAKE) -C $(top_builddir)/enigma/src/core/xml

$(top_builddir)/edvb/miniqt/libminiqt.la:
	$(MAKE) -C $(top_builddir)/edvb/miniqt

