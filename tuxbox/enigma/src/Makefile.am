INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/enigma/src/core/gui \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/include

bin_PROGRAMS = enigma

enigma_SOURCES = \
	bselect.cpp bselect.moc.cpp enigma.cpp enigma.moc.cpp enigma_dyn.cpp \
	enigma_event.cpp enigma_event.moc.cpp enigma_lcd.cpp enigma_lcd.moc.cpp \
	enigma_main.cpp enigma_main.moc.cpp enigma_plugins.cpp enigma_plugins.moc.cpp \
	enigma_setup.cpp enigma_setup.moc.cpp enigma_xmlrpc.cpp scan.cpp \
	scan.moc.cpp \
	setupnetwork.cpp setupnetwork.moc.cpp \
	setupvideo.cpp setupvideo.moc.cpp showbnversion.cpp \
	showbnversion.moc.cpp sselect.cpp sselect.moc.cpp streaminfo.cpp \
	streaminfo.moc.cpp enigma_mainmenu.cpp enigma_mainmenu.moc.cpp

enigma_LDADD = \
	$(top_builddir)/enigma/src/core/driver/libdriver.la \
	$(top_builddir)/enigma/src/core/dvb/libdvb.la \
	$(top_builddir)/enigma/src/core/dvb/lowlevel/liblowlevel.la \
	$(top_builddir)/enigma/src/core/gui/libgui.la \
	$(top_builddir)/enigma/src/core/gdi/libgdi.la \
	$(top_builddir)/enigma/src/core/system/libsystem.la \
	$(top_builddir)/enigma/src/core/xml/libxml.la \
	$(top_builddir)/edvb/miniqt/libminiqt.a \
	$(LIBFREETYPE_LIBS) \
	-ldl -lpng -lpthread

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS)

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

$(top_builddir)/edvb/miniqt/libminiqt.a:
	$(MAKE) -C $(top_builddir)/edvb/miniqt

%.moc.cpp: %.h
	$(MOC) $< -o $@

clean-local:
	-rm *.moc.cpp
