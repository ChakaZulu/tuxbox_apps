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
	neutrino_menu.cpp themes.cpp \
	bouquetlist.cpp channellist.cpp eventlist.cpp pluginlist.cpp \
	color.cpp favorites.cpp sleeptimer.cpp screensetup.cpp \
	epgview.cpp infoviewer.cpp scan.cpp update.cpp \
	filebrowser.cpp audioplayer.cpp nfs.cpp pictureviewer.cpp \
	rc_lock.cpp bookmarkmanager.cpp \
	timeosd.cpp epgplus.cpp epg_menu.cpp personalize.cpp \
	streaminfo2.cpp plugins.cpp imageinfo.cpp \
	audio_select.cpp movieinfo.cpp

if ENABLE_MOVIEPLAYER2
libneutrino_gui_a_SOURCES += movieplayer2.cpp
else
libneutrino_gui_a_SOURCES += \
	movieviewer.cpp movieplayer.cpp moviebrowser.cpp
endif

#	experimental_menu.cpp
#	ch_mosaic.cpp

if BOXTYPE_DREAMBOX
libneutrino_gui2_a_SOURCES = \
	motorcontrol.cpp
else
libneutrino_gui2_a_SOURCES = \
	alphasetup.cpp motorcontrol.cpp
endif

libtimerlist_a_SOURCES = \
	timerlist.cpp

if ENABLE_UPNP
INCLUDES += \
	@XMLTREE_CFLAGS@ \
	@UPNPCLIENT_CFLAGS@

libneutrino_gui_a_SOURCES += \
	upnpbrowser.cpp
endif

if ENABLE_MOVIEPLAYER2
AM_CPPFLAGS = -DMOVIEPLAYER2
endif
#AM_CPPFLAGS = -DSKIP_CA_STATUS
