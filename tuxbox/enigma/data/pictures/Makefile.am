SUBDIRS = mainmenu chk_stone chk_v2 neutrino shortcut

EXTRA_DIST = \
	checked.png format_on_small.png dolby_on.png \
	lcdmenu.png unchecked.png video.png button_red.png button_blue.png \
	button_yellow.png button_gray.png button_green.png checked_lcd.png \
	unchecked_lcd.png lcd_shutdown.png lcd_standby.png stereo_on.png \
	crypt_on_small.png mute-fs8.png icon-warning-fs8.png icon-error-fs8.png \
	icon-info-fs8.png icon-question-fs8.png volumeslider-fs8.png \
	folder-fs8.png clock-fs8.png clock_rec-fs8.png event_ok-fs8.png \
	event_failed-fs8.png dreamlogo-fs8.png
	
install-data-local:
	install -d $(DATADIR)/enigma/pictures
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/pictures
