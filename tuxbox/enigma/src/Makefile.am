INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/enigma/src/core/gui \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/xml

bin_PROGRAMS = enigma

enigma_SOURCES = \
	bselect.cpp bselect.moc.cpp enigma.cpp enigma.moc.cpp enigma_dyn.cpp \
	enigma_event.cpp enigma_event.moc.cpp enigma_lcd.cpp enigma_lcd.moc.cpp \
	enigma_main.cpp enigma_main.moc.cpp enigma_plugins.cpp enigma_plugins.moc.cpp \
	enigma_setup.cpp enigma_setup.moc.cpp enigma_xmlrpc.cpp scan.cpp \
	scan.moc.cpp setup_osd.cpp setup_osd.moc.cpp \
	setupnetwork.cpp setupnetwork.moc.cpp \
	setupvideo.cpp setupvideo.moc.cpp showbnversion.cpp \
	showbnversion.moc.cpp sselect.cpp sselect.moc.cpp streaminfo.cpp \
	streaminfo.moc.cpp

enigma_LDADD = \
	$(top_builddir)/enigma/src/core/driver/libdriver.a \
	$(top_builddir)/enigma/src/core/dvb/libdvb.a \
	$(top_builddir)/enigma/src/core/dvb/lowlevel/liblowlevel.a \
	$(top_builddir)/enigma/src/core/gui/libgui.a \
	$(top_builddir)/enigma/src/core/gdi/libgdi.a \
	$(top_builddir)/enigma/src/core/system/libsystem.a \
	$(top_builddir)/enigma/src/core/xml/libxml.a \
	$(top_builddir)/edvb/miniqt/libminiqt.a \
	$(LIBFREETYPE_LIBS) \
	-ldl -lpng -lpthread

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS)

$(top_builddir)/enigma/src/core/driver/libdriver.a:
	cd $(top_builddir)/enigma/src/core/driver && $(MAKE)

$(top_builddir)/enigma/src/core/dvb/libdvb.a:
	cd $(top_builddir)/enigma/src/core/dvb && $(MAKE)

$(top_builddir)/enigma/src/core/dvb/lowlevel/liblowlevel.a:
	cd $(top_builddir)/enigma/src/core/dvb/lowlevel && $(MAKE)

$(top_builddir)/enigma/src/core/gdi/libgdi.a:
	cd $(top_builddir)/enigma/src/core/gdi && $(MAKE)

$(top_builddir)/enigma/src/core/gui/libgui.a:
	cd $(top_builddir)/enigma/src/core/gui && $(MAKE)

$(top_builddir)/enigma/src/core/system/libsystem.a:
	cd $(top_builddir)/enigma/src/core/system && $(MAKE)

$(top_builddir)/enigma/src/core/xml/libxml.a:
	cd $(top_builddir)/enigma/src/core/xml && $(MAKE)

$(top_builddir)/edvb/miniqt/libminiqt.a:
	cd $(top_builddir)/edvb/miniqt && $(MAKE)

%.moc.cpp: %.h
	$(MOC) $< -o $@

clean-local:
	-rm *.moc.cpp
