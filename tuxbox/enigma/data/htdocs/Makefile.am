SUBDIRS = stylesheets

EXTRA_DIST = \
	epgview.css knop_0.gif knop_1.gif knop_2.gif knop_3.gif  \
	knop_4.gif knop_5.gif knop_6.gif knop_7.gif knop_8.gif \
	knop_9.gif led_aan.gif led_uit.gif on.gif off.gif si.css \
	speak_off.gif speak_on.gif topbalk.png trans.gif squ.png

install-data-local:
	install -d $(DATADIR)/enigma/htdocs
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/htdocs
