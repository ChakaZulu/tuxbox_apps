SUBDIRS = stylesheets

EXTRA_DIST = \
	webif.css filler.png led_aan.gif led_uit.gif on.gif off.gif  \
	speak_off.gif speak_on.gif topbalk.png trans.gif squ.png kalarm.png

install-data-local:
	install -d $(DATADIR)/enigma/htdocs
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/htdocs
