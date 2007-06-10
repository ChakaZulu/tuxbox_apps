SUBDIRS = widget bedit

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/src \
	@CURL_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@MPEGTOOLS_CFLAGS@ \
	@TUXTXT_CFLAGS@ \
	-I$(top_srcdir)/lib

noinst_LIBRARIES = libtimerlist.a libneutrino_gui.a libneutrino_gui2.a

libneutrino_gui_a_SOURCES = \
	bouquetlist.cpp channellist.cpp eventlist.cpp pluginlist.cpp \
	color.cpp favorites.cpp sleeptimer.cpp screensetup.cpp \
	epgview.cpp infoviewer.cpp scan.cpp update.cpp \
	filebrowser.cpp audioplayer.cpp nfs.cpp pictureviewer.cpp \
	movieplayer.cpp rc_lock.cpp bookmarkmanager.cpp \
	timeosd.cpp epgplus.cpp epg_menu.cpp \
	streaminfo2.cpp plugins.cpp imageinfo.cpp \
	audio_select.cpp \
	moviebrowser.cpp movieinfo.cpp

	
#	experimental_menu.cpp
#	ch_mosaic.cpp

libneutrino_gui2_a_SOURCES = \
	alphasetup.cpp motorcontrol.cpp

libtimerlist_a_SOURCES = \
	timerlist.cpp

if ENABLE_UPNP
INCLUDES += \
	@XMLTREE_CFLAGS@ \
	@UPNPCLIENT_CFLAGS@

libneutrino_gui_a_SOURCES += \
	upnpbrowser.cpp
endif

#AM_CPPFLAGS = -DSKIP_CA_STATUS
