SUBDIRS = chk chk_v2 decoration

EXTRA_DIST = \
	ch-bar.png checked.png ch-num.png crypt_off.png crypt_on.png \
	dolby_off.png dolby_on.png enigma2.png format_off.png format_on.png \
	lcdmenu.png unchecked.png video.png button_red.png button_blue.png \
	button_yellow.png button_gray.png button_green.png checked_lcd.png \
	unchecked_lcd.png
	
install-data-local:
	install -d $(DATADIR)/enigma/pictures
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/pictures
