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

neutrino_LDADD = \
	daemonc/libneutrino_daemonc.a \
	gui/bedit/libneutrino_gui_bedit.a \
	gui/libtimerlist.a \
	gui/libneutrino_gui.a \
	gui/widget/libneutrino_gui_widget.a \
	gui/widget/libneutrino_gui_widget2.a \
	driver/pictureviewer/libneutrino_pictureviewer.a \
	driver/libneutrino_driver.a \
	driver/audiodec/libneutrino_driver_audiodec.a \
	driver/libneutrino_driver_netfile.a \
	system/libneutrino_system.a \
	gui/libneutrino_gui2.a \
	$(top_builddir)/lib/controldclient/libcontroldclient.la \
	$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la \
	$(top_builddir)/lib/timerdclient/libtimerdclient.la \
	$(top_builddir)/lib/irsend/libirsend.la \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@CRAMFS_LIBS@ \
	@CURL_LIBS@ \
	@FREETYPE_LIBS@ \
	@LIRCDCLIENT_LIBS@ \
	@NET_LIBS@ \
	@UCODES_LIBS@ \
	@ZAPIT_LIBS@ \
	@LCDDISPLAY_LIBS@ \
	@PNG_LIBS@ \
	@MAD_LIBS@ \
	@ID3TAG_LIBS@ \
	@MPEGTOOLS_LIBS@ \
	@VORBISIDEC_LIBS@ \
	@TUXTXT_LIBS@ \
	@XMLTREE_LIBS@ \
	@UPNPCLIENT_LIBS@ \
	-lresolv \
	-ljpeg \
	-lpthread \
	@XML_LIBS@

AM_CPPFLAGS =

if ENABLE_UPNP
INCLUDES += @UPNPCLIENT_CFLAGS@
AM_CPPFLAGS += -DENABLE_UPNP
endif

#AM_CPPFLAGS = -DSKIP_CA_STATUS
