INCLUDES = \
	-I$(top_srcdir)/include

bin_PROGRAMS = enigma

enigma_SOURCES = \
	enigma.cpp enigma_dyn.cpp enigma_event.cpp enigma_lcd.cpp \
	enigma_main.cpp enigma_plugins.cpp enigma_setup.cpp enigma_xmlrpc.cpp scan.cpp \
	setupnetwork.cpp setupvideo.cpp showbnversion.cpp sselect.cpp streaminfo.cpp \
	enigma_mainmenu.cpp satconfig.cpp epgwindow.cpp setup_language.cpp enigma_info.cpp \
	enigma_scan.cpp setupskin.cpp setup_osd.cpp timer.cpp setup_lcd.cpp enigma_vcr.cpp \
	setup_rc.cpp channelinfo.cpp setup_harddisk.cpp enigma_ci.cpp upgrade.cpp satfind.cpp \
	rotorconfig.cpp setup_rfmod.cpp

enigma_LDADD = \
	$(top_builddir)/lib/driver/libenigma_driver.la \
	$(top_builddir)/lib/base/libenigma_base.la \
	$(top_builddir)/lib/codecs/libenigma_codecs.la \
	$(top_builddir)/lib/dvb/libenigma_dvb.la \
	$(top_builddir)/lib/dvb/lowlevel/libenigma_dvb_lowlevel.la \
	$(top_builddir)/lib/gui/libenigma_gui.la \
	$(top_builddir)/lib/gdi/libenigma_gdi.la \
	$(top_builddir)/lib/socket/libenigma_socket.la \
	$(top_builddir)/lib/system/libenigma_system.la \
	@MD5SUM_LIBS@ \
	@SIGC_LIBS@ \
	@XMLTREE_LIBS@ \
	-ldl -lpthread -lid3tag -lmad -lcrypt -lresolv -ltuxbox
#\
#	/dbox2/cdkroot/lib/libmpatrolmt.a -lpthread -lbfd -liberty
