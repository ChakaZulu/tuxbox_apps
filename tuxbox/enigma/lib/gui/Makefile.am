INCLUDES = \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/apps/enigma \
	-I$(top_srcdir)/enigma/src/core/dvb

noinst_LTLIBRARIES = libgui.la

libgui_la_SOURCES = \
	ebutton.cpp ebutton.moc.cpp echeckbox.cpp echeckbox.moc.cpp \
	elabel.cpp elabel.moc.cpp elbwindow.cpp elbwindow.moc.cpp elistbox.cpp \
	elistbox.moc.cpp emessage.cpp emessage.moc.cpp enumber.cpp \
	enumber.moc.cpp epixmap.cpp eprogress.cpp eprogress.moc.cpp eskin.cpp \
	eskin_register.cpp ewidget.cpp ewidget.moc.cpp ewindow.cpp \
	ewindow.moc.cpp multipage.cpp actions.cpp

libgui_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS)

%.moc.cpp: %.h
	$(MOC) $< -o $@

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libgui.la $(LIBDIR)/enigma

clean-local:
	-rm *.moc.cpp
