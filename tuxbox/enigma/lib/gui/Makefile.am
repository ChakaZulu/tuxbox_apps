INCLUDES = \
	-I$(top_srcdir)/enigma/src

lib_LTLIBRARIES = libenigma_gui.la

libenigma_gui_la_SOURCES = \
	ebutton.cpp echeckbox.cpp elabel.cpp emessage.cpp \
	enumber.cpp eprogress.cpp eskin.cpp listbox.cpp eskin_register.cpp \
	ewidget.cpp ewindow.cpp multipage.cpp actions.cpp guiactions.cpp \
	decoration.cpp statusbar.cpp

AM_CXXFLAGS = $(LIBFREETYPE_CFLAGS) $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

