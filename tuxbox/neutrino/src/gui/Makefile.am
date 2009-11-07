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
	-I$(top_srcdir)/lib

if ENABLE_INTERNAL_TUXTXT
INCLUDES += \
	@TUXTXT_CFLAGS@
endif

noinst_LIBRARIES = libtimerlist.a libneutrino_gui.a libneutrino_gui2.a libneutrino_gui3.a

libneutrino_gui_a_SOURCES = \
	audio_select.cpp \
	audio_setup.cpp \
	bookmarkmanager.cpp \
	bouquetlist.cpp \
	channellist.cpp \
	color.cpp \
	epg_menu.cpp \
	epgplus.cpp \
	epgview.cpp \
	eventlist.cpp \
	favorites.cpp \
	filebrowser.cpp \
	imageinfo.cpp \
	infoviewer.cpp \
	neutrino_menu.cpp \
	nfs.cpp \
	personalize.cpp \
	pluginlist.cpp \
	plugins.cpp \
	rc_lock.cpp \
	scan.cpp \
	scan_setup.cpp \
	screensetup.cpp \
	sleeptimer.cpp \
	software_update.cpp \
	streaminfo2.cpp \
	themes.cpp \
	timeosd.cpp \
	update.cpp \
	video_setup.cpp
	

libneutrino_gui3_a_SOURCES =  movieinfo.cpp

if ENABLE_GUI_MOUNT
libneutrino_gui_a_SOURCES += nfs.cpp
endif

if ENABLE_ESD
libneutrino_gui_a_SOURCES += esound.cpp esd_setup.cpp mediaplayer_setup.cpp
endif

if ENABLE_AUDIOPLAYER
libneutrino_gui_a_SOURCES += \
	audioplayer.cpp audioplayer_setup.cpp mediaplayer_setup.cpp
endif

if ENABLE_PICTUREVIEWER
libneutrino_gui_a_SOURCES += \
	pictureviewer.cpp pictureviewer_setup.cpp mediaplayer_setup.cpp
endif

if ENABLE_MOVIEPLAYER
libneutrino_gui_a_SOURCES += moviebrowser.cpp movieplayer_setup.cpp mediaplayer_setup.cpp
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
