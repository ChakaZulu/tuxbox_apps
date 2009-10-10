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

noinst_LIBRARIES = libtimerlist.a libneutrino_gui.a libneutrino_gui2.a libneutrino_gui3.a

libneutrino_gui_a_SOURCES = \
	neutrino_menu.cpp themes.cpp \
	bouquetlist.cpp channellist.cpp eventlist.cpp pluginlist.cpp \
	color.cpp favorites.cpp sleeptimer.cpp screensetup.cpp \
	epgview.cpp infoviewer.cpp scan.cpp update.cpp \
	filebrowser.cpp \
	rc_lock.cpp bookmarkmanager.cpp \
	timeosd.cpp epgplus.cpp epg_menu.cpp personalize.cpp \
	streaminfo2.cpp plugins.cpp imageinfo.cpp \
	audio_select.cpp software_update.cpp

libneutrino_gui3_a_SOURCES =  movieinfo.cpp

if ENABLE_GUI_MOUNT
libneutrino_gui_a_SOURCES += nfs.cpp
endif

if ENABLE_ESD
libneutrino_gui_a_SOURCES += esound.cpp
endif

if ENABLE_AUDIOPLAYER
libneutrino_gui_a_SOURCES += \
	audioplayer.cpp
endif

if ENABLE_PICTUREVIEWER
libneutrino_gui_a_SOURCES += \
	pictureviewer.cpp
endif

if ENABLE_MOVIEPLAYER
libneutrino_gui_a_SOURCES += moviebrowser.cpp
if ENABLE_MOVIEPLAYER2
libneutrino_gui_a_SOURCES += movieplayer2.cpp
else
libneutrino_gui_a_SOURCES += \
	movieviewer.cpp movieplayer.cpp
endif
endif

#	experimental_menu.cpp
#	ch_mosaic.cpp

libneutrino_gui2_a_SOURCES = \
	motorcontrol.cpp

if BOXTYPE_DBOX2
libneutrino_gui2_a_SOURCES += \
	alphasetup.cpp 
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
