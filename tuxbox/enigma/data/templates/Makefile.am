EXTRA_DIST = eit.tmp index.tmp aboutDMM.tmp linksOfficialSites.tmp linksOtherSites.tmp linksForums.tmp header.tmp timerList.tmp \
	     epg.tmp mepg.tmp videocontrols.tmp audioSelection.tmp editTimerEvent.tmp sendMessage.tmp configHDD.tmp configUSB.tmp

install-data-local:
	install -d $(DATADIR)/enigma/templates
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/templates
