EXTRA_DIST = eit.tmp index.tmp helpDMMSites.tmp helpOtherSites.tmp helpForums.tmp header.tmp timerList.tmp \
	     timerListBody.tmp epg.tmp mepg.tmp videocontrols.tmp audioSelection.tmp editTimerEvent.tmp sendMessage.tmp \
	     configHDD.tmp configUSB.tmp msgWindow.tmp zap.tmp rec.tmp index2.tmp blank.tmp epgDetails.tmp \
	     index_small.tmp index_big.tmp eit_small.tmp addTimerEvent.tmp wap.tmp wapepg.tmp wapzap.tmp \
	wapEPGDetails.tmp wapTimerList.tmp queryEditTimer.tmp queryDeleteTimer.tmp deleteTimerComplete.tmp \
	mountPoint.tmp mountPoints.tmp mountPointWindow.tmp subChannelSelection.tmp

install-data-local:
	install -d $(DATADIR)/enigma/templates
	install -m 0644 $(EXTRA_DIST) $(DATADIR)/enigma/templates
