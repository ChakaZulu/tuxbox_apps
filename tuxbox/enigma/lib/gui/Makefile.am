INCLUDES = \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/base \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/apps/enigma \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/gui \
	-I$(top_srcdir)/enigma/src/include

noinst_LTLIBRARIES = libgui.la

libgui_la_SOURCES = \
	ebutton.cpp echeckbox.cpp elabel.cpp elbwindow.cpp elistbox.cpp \
	emessage.cpp enumber.cpp eprogress.cpp eskin.cpp \
	eskin_register.cpp ewidget.cpp ewindow.cpp multipage.cpp actions.cpp \
	esize.cpp epoint.cpp erect.cpp
	
libgui_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libgui.la $(LIBDIR)/enigma

