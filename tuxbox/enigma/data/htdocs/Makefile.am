EXTRA_DIST = \
	webif.css filler.png led_aan.gif led_uit.gif on.gif off.gif  \
	speak_off.gif speak_on.gif topbalk.png trans.gif squ.png kalarm.png \
	index.js epg.js player_eject.png player_end.png player_fwd.png \
	player_pause.png player_play.png player_rew.png player_start.png \
	player_stop.png

install-data-local:
	install -d $(DATADIR)/enigma/htdocs
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/htdocs
