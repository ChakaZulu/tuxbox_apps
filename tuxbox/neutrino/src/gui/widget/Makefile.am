INCLUDES = \
	-I@DRIVER@/include \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I$(top_srcdir)/libs/libcontroldclient \
	-I$(top_srcdir)/libs/liblcddclient \
	-I$(top_srcdir)/libs/libsectionsdclient \
	-I$(top_srcdir)/libs/libtimerdclient

noinst_LIBRARIES = libneutrino_gui_widget_basic.a libneutrino_gui_widget.a libneutrino_gui_widget2.a 

libneutrino_gui_widget_a_SOURCES = \
	colorchooser.cpp keychooser.cpp \
	menue.cpp stringinput.cpp stringinput_ext.cpp \
	messagebox.cpp hintbox.cpp listbox.cpp \
	lcdcontroler.cpp
	
libneutrino_gui_widget2_a_SOURCES = \
	progresswindow.cpp

libneutrino_gui_widget_basic_a_SOURCES = \
        component.cpp window.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

