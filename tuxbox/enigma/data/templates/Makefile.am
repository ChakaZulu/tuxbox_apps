EXTRA_DIST = eit.tmp index.tmp helpDMMSites.tmp helpOtherSites.tmp helpForums.tmp \
	mepg.tmp audioSelection.tmp editTimerEvent.tmp sendMessage.tmp \
	configSwapFile.tmp msgWindow.tmp zap.tmp movies.tmp index2.tmp rotorSat.tmp \
	index_small.tmp index_big.tmp eit_small.tmp addTimerEvent.tmp wap.tmp wapepg.tmp wapzap.tmp \
	wapEPGDetails.tmp wapTimerList.tmp queryEditTimer.tmp queryDeleteTimer.tmp deleteTimerComplete.tmp \
	mountPoint.tmp mountPoints.tmp mountPointWindow.tmp subChannelSelection.tmp mepg_small.tmp \
	pdaResponse.tmp flashMgr.tmp configSettings.tmp remoteControl.tmp \
	pdaRemoteControl.tmp sat.tmp satFinder.tmp remoteControlDbox2.tmp leftnavi.tmp rotor.tmp \
	root.tmp flashMgrReadMenu.tmp flashMgrWriteMenu.tmp mtd.tmp flashMgrProgressData.tmp flashMgrProgress.tmp \
	pdaRemoteControlDbox2.tmp configSwapFileMenu.tmp movieSources.tmp \
	XMLStreaminfo.tmp streaminfo_satellite.xsl streaminfo_cable.xsl streaminfo_terrestrial.xsl serviceepg.xsl \
	XMLCurrentServiceData.tmp \
	dvrcontrols.tmp webxtv.tmp \
	XMLBoxInfo.tmp  HTMLBoxInfo.tmp \
	XMLData.tmp HTMLData.tmp \
	XMLServiceEPG.tmp XMLEPGEntry.tmp HTMLServiceEPG.tmp HTMLEPGEntry.tmp \
	XMLTimerList.tmp XMLTimerEntry.tmp XMLTimerListBody.tmp HTMLTimerList.tmp HTMLTimerEntry.tmp HTMLTimerListBody.tmp

install-data-local:
	install -d $(DESTDIR)$(TUXBOXDATADIR)/enigma/templates
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(TUXBOXDATADIR)/enigma/templates
