INCLUDES = \
	-I$(top_srcdir)/include

bin_PROGRAMS = enigma

enigma_SOURCES = \
	enigma.cpp enigma_dyn.cpp enigma_event.cpp enigma_lcd.cpp parentallock.cpp \
	enigma_main.cpp enigma_plugins.cpp enigma_setup.cpp enigma_xmlrpc.cpp scan.cpp \
	setupnetwork.cpp setupvideo.cpp sselect.cpp streaminfo.cpp enigma_dyn_rotor.cpp\
	enigma_mainmenu.cpp satconfig.cpp epgwindow.cpp setup_timezone.cpp enigma_info.cpp \
	enigma_scan.cpp setupskin.cpp setup_osd.cpp timer.cpp setup_lcd.cpp enigma_vcr.cpp \
	channelinfo.cpp setup_harddisk.cpp enigma_ci.cpp upgrade.cpp satfind.cpp setup_rc.cpp \
	setup_extra.cpp rotorconfig.cpp setup_rfmod.cpp wizard_language.cpp wizard_scan.cpp \
	engrab.cpp setupengrab.cpp helpwindow.cpp tpeditwindow.cpp enigma_web.cpp enigma_epg.cpp \
	wizard_timezone.cpp elirc.cpp hotplug.cpp enigma_bouquet.cpp setup_window.cpp \
	system_settings.cpp time_settings.cpp software_update.cpp enigma_standby.cpp \
	enigma_mmisocket.cpp enigma_mmi.cpp flashtool.cpp time_correction.cpp \
	audio_dynamic.cpp setup_keyboard.cpp wizard_fpupdate.cpp enigma_mount.cpp \
	enigma_dyn_mount.cpp enigma_dyn_utils.cpp enigma_dyn_wap.cpp enigma_dyn_conf.cpp \
	enigma_dyn_flash.cpp enigma_dyn_xml.cpp enigma_streamer.cpp \
	enigma_processutils.cpp enigma_picmanager.cpp

enigma_LDADD_WHOLE = \
	$(top_builddir)/lib/driver/libenigma_driver.a \
	$(top_builddir)/lib/base/libenigma_base.a \
	$(top_builddir)/lib/codecs/libenigma_codecs.a \
	$(top_builddir)/lib/dvb/libenigma_dvb.a \
	$(top_builddir)/lib/dvb/lowlevel/libenigma_dvb_lowlevel.a \
	$(top_builddir)/lib/gui/libenigma_gui.a \
	$(top_builddir)/lib/gdi/libenigma_gdi.a \
	$(top_builddir)/lib/socket/libenigma_socket.a \
	$(top_builddir)/lib/system/libenigma_system.a \
	$(top_builddir)/lib/picviewer/libenigma_picviewer.a

enigma_LDADD = \
	@FREETYPE_LIBS@ \
	@ID3TAG_LIBS@ \
	@MAD_LIBS@ \
	@MD5SUM_LIBS@ \
	@PNG_LIBS@ \
	@SIGC_LIBS@ \
	@XMLTREE_LIBS@ \
	@TUXBOX_LIBS@ \
	@CONFIGFILE_LIBS@ \
	@NET_LIBS@ \
	-ldl -lpthread -lcrypt -lresolv -ljpeg -lungif -ltuxtxt

enigma$(EXEEXT): $(enigma_OBJECTS) $(enigma_DEPENDENCIES) $(enigma_LDADD_WHOLE)
	@rm -f enigma$(EXEEXT)
	$(CXXLINK) $(enigma_LDFLAGS) -Wl,--export-dynamic $(enigma_OBJECTS) -Wl,--whole-archive $(enigma_LDADD_WHOLE) -Wl,--no-whole-archive $(enigma_LDADD) $(LIBS)
