SUBDIRS = gui daemonc driver system icons locale

INCLUDES = \
	-I@DRIVER@/include \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I$(top_srcdir)/daemons/timerd \
	-I$(top_srcdir)/libs/libcontroldclient \
	-I$(top_srcdir)/libs/liblcddclient \
	-I$(top_srcdir)/libs/libsectionsdclient \
	-I$(top_srcdir)/libs/libtimerdclient

bin_PROGRAMS = neutrino

neutrino_SOURCES = neutrino.cpp

neutrino_LDADD = \
	daemonc/libneutrino_daemonc.a \
	driver/libneutrino_driver.a \
	gui/bedit/libneutrino_gui_bedit.a \
	gui/widget/libneutrino_gui_widget.a \
	gui/libneutrino_gui.a \
	gui/widget/libneutrino_gui_widget2.a \
	gui/widget/libneutrino_gui_widget_basic.a \
	system/libneutrino_system.a \
	gui/libneutrino_gui2.a \
	@CONFIGFILE_LIBS@ \
	@CRAMFS_LIBS@ \
	@MD5SUM_LIBS@ \
	@NET_LIBS@ \
	@UCODES_LIBS@ \
	@ZAPITCLIENT_LIBS@ \
	$(top_builddir)/libs/libcontroldclient/libcontroldclient.la \
	$(top_builddir)/libs/liblcddclient/liblcddclient.la \
	$(top_builddir)/libs/libsectionsdclient/libsectionsdclient.la \
	$(top_builddir)/libs/libtimerdclient/libtimerdclient.la \
	@FREETYPE_LIBS@ \
	@CURL_LIBS@ \
	-lpthread -lz

AM_CPPFLAGS = -D_GNU_SOURCE

