SUBDIRS = widget bedit

INCLUDES = \
	-I@DRIVER@/include \
	@CURL_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I$(top_srcdir)/daemons/timerd \
	-I$(top_srcdir)/libs/libcontroldclient \
	-I$(top_srcdir)/libs/liblcddclient \
	-I$(top_srcdir)/libs/libsectionsdclient \
	-I$(top_srcdir)/libs/libtimerdclient \
	-I$(top_srcdir)/gui

noinst_LIBRARIES = libneutrino_gui.a libneutrino_gui2.a

libneutrino_gui_a_SOURCES = \
	bouquetlist.cpp channellist.cpp eventlist.cpp gamelist.cpp \
	color.cpp favorites.cpp sleeptimer.cpp screensetup.cpp \
	epgview.cpp infoviewer.cpp scan.cpp update.cpp dboxinfo.cpp

libneutrino_gui2_a_SOURCES = \
	streaminfo.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

