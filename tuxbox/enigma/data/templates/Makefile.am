EXTRA_DIST = eit.tmp index.tmp helpDMMSites.tmp helpOtherSites.tmp helpForums.tmp header.tmp timerList.tmp \
	     timerListBody.tmp epg.tmp mepg.tmp videocontrols.tmp audioSelection.tmp editTimerEvent.tmp sendMessage.tmp \
	     configHDD.tmp configUSB.tmp msgWindow.tmp

install-data-local:
	install -d $(DATADIR)/enigma/templates
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/templates
