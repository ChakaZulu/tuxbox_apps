SUBDIRS = gui daemonc driver system

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/lib \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@LCDDISPLAY_CFLAGS@ \
	@CONFIGFILE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@TUXTXT_CFLAGS@

bin_PROGRAMS = neutrino

neutrino_SOURCES = neutrino.cpp

AM_CPPFLAGS =

neutrino_LDADD = \
	daemonc/libneutrino_daemonc.a \
	gui/libtimerlist.a \
	gui/libneutrino_gui.a \
	gui/bedit/libneutrino_gui_bedit.a \
	gui/widget/libneutrino_gui_widget.a \
	gui/widget/libneutrino_gui_widget2.a \
	driver/libneutrino_driver.a \
	gui/libneutrino_gui3.a \
	gui/widget/libneutrino_gui_widget3.a \
	system/libneutrino_system.a \
	gui/libneutrino_gui2.a \
	$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la \
	$(top_builddir)/lib/timerdclient/libtimerdclient.la \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@CURL_LIBS@ \
	@FREETYPE_LIBS@ \
	@NET_LIBS@ \
	@UCODES_LIBS@ \
	@ZAPIT_LIBS@ \
	@LCDDISPLAY_LIBS@ \
	@MPEGTOOLS_LIBS@ \
	@TUXTXT_LIBS@ \
	@XMLTREE_LIBS@ \
	@UPNPCLIENT_LIBS@ \
	-lresolv \
	-lpthread \
	-ldl \
	@XML_LIBS@

if ENABLE_LIRC
neutrino_LDADD += \
	$(top_builddir)/lib/irsend/libirsend.la \
	@LIRCDCLIENT_LIBS@
endif

if ENABLE_AUDIOPLAYER

if ENABLE_FLAC
FLAClib = -lFLAC
AM_CPPFLAGS += -DENABLE_FLAC
else
FLAClib =
endif

neutrino_LDADD += \
	driver/audiodec/libneutrino_driver_audiodec.a \
	@MAD_LIBS@ \
	@ID3TAG_LIBS@ \
	@VORBISIDEC_LIBS@ \
	$(FLAClib)

if ENABLE_INTERNETRADIO
neutrino_LDADD += \
	driver/libneutrino_driver_netfile.a
endif

endif

if ENABLE_PICTUREVIEWER
neutrino_LDADD += \
	driver/pictureviewer/libneutrino_pictureviewer.a \
	@PNG_LIBS@ \
	-ljpeg
endif

if ENABLE_UPNP
INCLUDES += @UPNPCLIENT_CFLAGS@
AM_CPPFLAGS += -DENABLE_UPNP
endif
