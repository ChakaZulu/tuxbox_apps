INCLUDES = \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src \
	-I$(top_srcdir)/enigma/src/core/base \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/apps/enigma \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/gui \
	-I$(top_srcdir)/enigma/src/include

lib_LTLIBRARIES = libenigma_gui.la

libenigma_gui_la_SOURCES = \
	ebutton.cpp echeckbox.cpp elabel.cpp elbwindow.cpp elistbox.cpp \
	emessage.cpp enumber.cpp eprogress.cpp eskin.cpp listbox.cpp \
	eskin_register.cpp ewidget.cpp ewindow.cpp multipage.cpp actions.cpp \
	guiactions.cpp

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

