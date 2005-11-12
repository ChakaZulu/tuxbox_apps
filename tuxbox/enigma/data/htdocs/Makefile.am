EXTRA_DIST = \
	webif.css led_on.gif led_off.gif on.gif off.gif  \
	speak_off.gif speak_on.gif topbalk.png trans.gif squ.png timer.gif \
	index.js epg.js trash.gif dm7000.jpg blinking_red.gif \
	format_on.png format_off.png dolby_on.png dolby_off.png \
	crypt_on.png crypt_off.png mountMgr.js rc_small.jpg rc_big.jpg \
	bg.png rc_dbox2.jpg dhtml.js rc_dbox2_small.jpg \
	mepg_grad.png edit.gif timer.js window.js topbalk2.png topbalk3.png \
	topbalk4.png zap.js zapdata.js up.gif down.gif topbalk_small.png \
	webif_small.css topbalk2_small.png topbalk3_small.png \
	topbalk4_small.png blue.png green.png red.png grey.png yellow.png \
	locked.png unlocked.png 1rightarrow.gif 1leftarrow.gif player_stop.gif \
	player_fwd.gif player_rew.gif dvr.gif movieplayer.js

install-data-local:
	install -d $(DESTDIR)$(TUXBOXDATADIR)/enigma/htdocs
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(TUXBOXDATADIR)/enigma/htdocs
