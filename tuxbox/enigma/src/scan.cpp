#include <time.h>
#include <scan.h>
#include <enigma.h>

#include <enigma_main.h>
#include <lib/base/i18n.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/si.h>
#include <lib/dvb/dvb.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ewindow.h>
#include <lib/gdi/font.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/emessage.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/decoder.h>
#include <lib/driver/rc.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/combobox.h>
#include <lib/gui/guiactions.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/dvbscan.h>
#include <lib/dvb/dvbservice.h>
#include <lib/system/info.h>

tsSelectType::tsSelectType(eWidget *parent)
	:eWidget(parent), check(NULL)
{
	init_tsSelectType();
}
void tsSelectType::init_tsSelectType()
{
	list=new eListBox<eListBoxEntryMenu>(this);
	list->setName("menu");
	BuildSkin("tsSelectType");

	list->setFlags(eListBox<eListBoxEntryText>::flagShowEntryHelp);

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feTerrestrial )
	{
		check=new eListBoxEntryCheck(list, _("Disable 5V"), "/elitedvb/DVB/config/disable_5V", _("disable 5V for passive terrerstrial antennas"));
		check->selected.connect( slot(*eFrontend::getInstance(), &eFrontend::setTerrestrialAntennaVoltage) );
		new eListBoxEntryMenuSeparator(list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	}
	new eListBoxEntryMenuItem(list, _("Automatic Transponder Scan"), (void*)2, 0, _("open automatic transponder scan") );
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		new eListBoxEntryMenuItem(list, _("Automatic Multisat Scan"), (void*)3, 0, _("open automatic multisat transponder scan") );
	new eListBoxEntryMenuItem(list, _("manual scan.."), (void*)1, 0, _("open manual transponder scan") );
	CONNECT(list->selected, tsSelectType::selected);
}

void tsSelectType::selected(eListBoxEntryMenu *entry)
{
	if ( entry && entry == check )
		return;
	if (entry && entry->getKey())
		close((int)entry->getKey());
	else
		close((int)TransponderScan::stateEnd);
}

int tsSelectType::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		setFocus(list);
		break;
	case eWidgetEvent::childChangedHelpText:
		return parent->eventHandler( event );
	default:
		return eWidget::eventHandler(event);
		break;
	}
	return 0;
}

tsManual::tsManual(eWidget *parent, const eTransponder &transponder, eWidget *LCDTitle, eWidget *LCDElement)
:eWidget(parent), transponder(transponder)
{
	init_tsManual(LCDTitle,LCDElement);
}
void tsManual::init_tsManual( eWidget *LCDTitle, eWidget *LCDElement)
{
#ifndef DISABLE_LCD
	setLCD(LCDTitle, LCDElement);
#endif
	int ft=0;
	switch (eSystemInfo::getInstance()->getFEType())
	{
	default:	
	case eSystemInfo::feSatellite:
		ft=eTransponderWidget::deliverySatellite;
		break;
	case eSystemInfo::feCable:
		ft=eTransponderWidget::deliveryCable;
		break;
	case eSystemInfo::feTerrestrial:
		ft=eTransponderWidget::deliveryTerrestrial;
		break;
	}

	transponder_widget=new eTransponderWidget(this, 1, ft);
	transponder_widget->setName("transponder");

	festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");

	c_useonit=CreateSkinnedCheckbox("useonit");
	
	c_usebat=CreateSkinnedCheckbox("usebat");
	
	c_onlyFree=CreateSkinnedCheckbox("onlyFree");

	c_searchnit=CreateSkinnedCheckbox("searchnit");

	CONNECT(CreateSkinnedButton("start")->selected, tsManual::start);
	CONNECT(CreateSkinnedButton("manual_pids")->selected, tsManual::manual_pids);

	BuildSkin("tsManual");

	transponder_widget->load();
	transponder_widget->setTransponder(&transponder);

	CONNECT(transponder_widget->updated, tsManual::retune);
	setHelpID(62);
}

void tsManual::start()
{
	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	if (!sapi)
	{
		eWarning("no scan active");
		close(1);
	} else
	{
		sapi->addTransponder(transponder);
		sapi->setUseONIT(c_useonit->isChecked());
		sapi->setUseBAT(c_usebat->isChecked());
		sapi->setNetworkSearch(c_searchnit->isChecked());
		sapi->setOnlyFree(c_onlyFree->isChecked());
		sapi->setSkipOtherOrbitalPositions(1);
		close(0);
	}
}

void tsManual::manual_pids()
{
	parent->hide();
	transponder.original_network_id=0;
	transponder.transport_stream_id=0;
	ManualPIDWindow wnd(&transponder);
#ifndef DISABLE_LCD
	wnd.setLCD(LCDTitle, LCDElement);
#endif
	wnd.show();
	wnd.exec();
	wnd.hide();
	parent->show();
}

void tsManual::retune()
{
	if (!transponder_widget->getTransponder(&transponder))
		transponder.tune();
}

tsTryLock::tsTryLock(eWidget *parent, tpPacket *packet, eString ttext)
	:eWidget(parent), ret(0), inProgress(0), packet(packet)
	,current_tp(packet->possibleTransponders.begin())	
{
	init_tsTryLock(parent,packet, ttext);
}
void tsTryLock::init_tsTryLock(eWidget *parent, tpPacket *packet, eString ttext)
{
	l_status=CreateSkinnedLabel("lStatus",0, RS_WRAP);

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");

	b_abort=CreateSkinnedButton("bAbort");

	BuildSkin("tsTryLock");

	eLabel *l = (eLabel*)search("lNet");
	if (l)
	{
		eString text = l->getText();
		text.erase( text.size()-3 );
		text+=ttext;
		l->setText(text);
	}
	CONNECT(eDVB::getInstance()->eventOccured, tsTryLock::dvbEvent);
	CONNECT(b_abort->selected, eWidget::reject);
}

void tsTryLock::dvbEvent(const eDVBEvent &event)
{
	if (!inProgress)
		return;
	switch (event.type)
	{
		case eDVBScanEvent::eventTunedIn:
			inProgress=0;
			if (event.err)
			{
				inProgress=1;
				if (nextTransponder(event.err))  
					close(-2); // no lock possible
				else
				{
					eString progress=_("Waiting for tuner lock on:");
					progress += eString().sprintf("\n\n%d / %d / %c",
					current_tp->satellite.frequency/1000,
					current_tp->satellite.symbol_rate/1000,
					current_tp->satellite.polarisation?'V':'H');

					static int i=0;
					i++;
					char bla [(i%5)+1];
					memset(bla, '.', i%5);
					bla[i%5]=0;

					progress += bla;
					l_status->setText(progress);
				}
			}
			else
				accept();  // tp found
			break;
	default:
		break;
	}
}

int tsTryLock::nextTransponder(int next)
{
	if (next)
	{
		if ( next != -EAGAIN )
		{
			current_tp->state=eTransponder::stateError;
			eDebug("set to state Error");
		}
		else
			eDebug("dont set to state error");
		++current_tp;
	}

	if (current_tp == packet->possibleTransponders.end())
	{
		if ( next == -EAGAIN )
			current_tp=packet->possibleTransponders.begin();
		else
		{
			inProgress=0;
			return 1;
		}
	}

	return current_tp->tune();
}

int tsTryLock::eventHandler(const eWidgetEvent &e)
{
	switch(e.type)
	{
		case eWidgetEvent::execBegin:
			inProgress=1;
			nextTransponder(0);
			setFocus(b_abort);
			break;
		case eWidgetEvent::wantClose:
			eFrontend::getInstance()->abortTune();
			if ( e.parameter == 1 )  // cancel
			{
				eWidgetEvent ev = e;
				ev.parameter=-1;
				return eWidget::eventHandler(ev);
			}
		default:
			break;
	}
	return eWidget::eventHandler(e);
}

tsAutomatic::tsAutomatic(eWidget *parent)
	:eWidget(parent), inProgress(0)
{
	init_tsAutomatic();
}
void tsAutomatic::init_tsAutomatic()
{
	l_network=CreateSkinnedComboBoxWithLabel("network", 3, "lNet");

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");
	
	l_status=CreateSkinnedLabel("status",0, RS_WRAP);

	c_onlyFree = CreateSkinnedCheckbox("onlyFree",0);
	c_onlyFree->hide();
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
	{
		CreateSkinnedCheckbox("nocircular",0,"/elitedvb/DVB/config/nocircular");
		c_nocircular->hide();
	}
	else
		c_nocircular=0;

	b_start=CreateSkinnedButton("start");
	b_start->hide();


	eString tmp = "tsAutomatic";

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		tmp+="_sat";
	else
		tmp+="_cable";

	BuildSkin(tmp.c_str());

	eDebug("build %s", tmp.c_str() );
//	l_network->setCurrent(new eListBoxEntryText(*l_network, _("automatic"), (void*)0, eTextPara::dirCenter) );

	CONNECT(b_start->selected, tsAutomatic::start);
	CONNECT(l_network->selchanged, tsAutomatic::networkSelected);

	CONNECT(eDVB::getInstance()->eventOccured, tsAutomatic::dvbEvent);

	if (loadNetworks())
		eFatal("loading networks failed");

	l_network->setCurrent( 0 );	

	switch (eSystemInfo::getInstance()->getFEType())
	{
		case eSystemInfo::feSatellite:
			l_status->setText(_("To begin searching for a valid satellite press OK, or choose your desired satellite manually and press OK"));
			break;
		case eSystemInfo::feCable:
			l_status->setText(_("To begin searching for a valid cable provider press OK, or choose your desired cable provider manually and press OK"));
			break;
		case eSystemInfo::feTerrestrial:
			l_status->setText(_("To begin searching for a valid transponder press OK, or choose your desired location manually and press OK"));
			break;
	}

	setFocus(l_network);
	setHelpID(61);
}

void tsAutomatic::start()
{
	int snocircular = c_nocircular ? c_nocircular->isChecked() : 0;
	eConfig::getInstance()->setKey("/elitedvb/DVB/config/nocircular",snocircular);    

	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	if (!sapi)
	{
		eWarning("no scan active");
		close(1);
	} else
	{
		tpPacket *pkt=(tpPacket*)(l_network->getCurrent()->getKey());
		eLNB *lnb=0;
		eSatellite *sat = eTransponderList::getInstance()->findSatellite( pkt->orbital_position );
		if ( sat )
			lnb = sat->getLNB();

		for (std::list<eTransponder>::iterator i(pkt->possibleTransponders.begin()); i != pkt->possibleTransponders.end(); ++i)
		{
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
			{
				if(snocircular)
					i->satellite.polarisation&=1;   // CEDR
				if ( lnb && !i->satellite.useable( lnb ) )
				{
					eDebug("skip %d", i->satellite.frequency );
					continue;
				}
			}
			sapi->addTransponder(*i);
		}

		// scanflags auswerten
		sapi->setUseONIT(pkt->scanflags & 4);
		sapi->setUseBAT(pkt->scanflags & 2);
		sapi->setNetworkSearch(pkt->scanflags & 1);

		// macht nur Probleme...bzw dauert recht lang...
		sapi->setSkipOtherOrbitalPositions(1);
		sapi->setOnlyFree(c_onlyFree->isChecked());
		sapi->setNoCircularPolarization(snocircular);
//		sapi->setClearList(1);

		close(0);
	}
}

void tsAutomatic::networkSelected(eListBoxEntryText *l)
{
	b_start->hide();
	if (nextNetwork(-1)) // if "automatic" selected,
	{
		automatic=1;
		nextNetwork();  // begin with first
	} else
		automatic=0;
	inProgress=1;
	tuneNext(0);
}

void tsAutomatic::dvbEvent(const eDVBEvent &event)
{
	if (!inProgress)
		return;
	switch (event.type)
	{
		case eDVBScanEvent::eventScanCompleted:
			close(0);
			break;            
		case eDVBScanEvent::eventTunedIn:
			inProgress=0;
			if (event.err)
			{
				inProgress=1;
				tuneNext(event.err);
			}
			else
			{
				if ( c_nocircular )
					c_nocircular->show();
				c_onlyFree->show();
				b_start->show();
				setFocus(c_onlyFree);
				l_status->setText(_("A valid transponder has been found. Verify that the right network is selected"));
			}
			break;
	default:
		break;
	}
}

int tsAutomatic::loadNetworks()
{
	int err;

	if(	(err = eTransponderList::getInstance()->reloadNetworks()) )
		return err;

	std::map<int,eSatellite*> sats;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			sats[s->getOrbitalPosition()]=s;

	for ( std::list<tpPacket>::const_iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
		if ( ( sats.find(i->orbital_position) != sats.end()) || (eSystemInfo::getInstance()->getFEType() != eSystemInfo::feSatellite) )
			new eListBoxEntryText(*l_network, i->name, (void*)&*i, eTextPara::dirCenter);

	return 0;
}

int tsAutomatic::nextNetwork(int first)
{
//	eDebug("next network");

	if (first != -1)
		l_network->moveSelection(first ? eListBox<eListBoxEntryText>::dirFirst : eListBox<eListBoxEntryText>::dirDown);

	tpPacket *pkt=(tpPacket*)(l_network->getCurrent() -> getKey());

//	eDebug("pkt: %p", pkt);

	if (!pkt)
		return -1;

	current_tp = pkt->possibleTransponders.begin();
	last_tp = pkt->possibleTransponders.end();
	first_tp = pkt->possibleTransponders.begin();
	return 0;
}

int tsAutomatic::nextTransponder(int next)
{
	if (next)
	{
		if ( next != -EAGAIN )
			current_tp->state=eTransponder::stateError;

		++current_tp;
	}

	if (current_tp == last_tp)
	{
		if ( next == -EAGAIN )
			current_tp=first_tp;
		else
		{
			inProgress=0;
			return 1;
		}
	}

	if ( c_nocircular && c_nocircular->isChecked() )
		current_tp->satellite.polarisation&=1;   // CEDR

	return current_tp->tune();
}

int tsAutomatic::tuneNext(int next)
{
	while (nextTransponder(next))
	{
		if (automatic)
		{
			if (nextNetwork())	// wrapped around?
			{
				l_status->setText(_("All known transponders have been tried,"
					" but no lock was possible. Verify antenna-/cable-setup or try manual search "
					"if its some obscure satellite/network."));
				return -1;
			}
		}
		else
		{
			l_status->setText(_("All known transponders have been tried,"
				" but no lock was possible. Verify antenna-/cable-setup or try another satellite/network."));
			return -1;
		}
		next=0;
	}

	static int i=0;
	i++;
	eString progress=_("Search in progress ");
	char bla [(i%5)+1];
	memset(bla, '.', i%5);
	bla[i%5]=0;
	progress += bla;
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
	{
		progress += eString().sprintf("\n%d / %d / %c",
			current_tp->satellite.frequency/1000,
			current_tp->satellite.symbol_rate/1000,
			current_tp->satellite.polarisation?'V':'H');
	}
	else if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feCable )
	{
		progress += eString().sprintf("\n%d / %d",
			current_tp->cable.frequency/1000,
			current_tp->cable.symbol_rate/1000 );
	}
	else if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feTerrestrial )
	{
		progress += eString().sprintf("\n%d Khz",
			current_tp->terrestrial.centre_frequency/1000);
	}
	l_status->setText(progress);

	return 0;
}

void tsAutomatic::openNetworkCombo()
{
	setFocus(l_network);
	l_network->onOkPressed();
}

int tsAutomatic::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::wantClose:
		eFrontend::getInstance()->abortTune();
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

tsText::tsText(eString sheadline, eString sbody, eWidget *parent)
	:eWidget(parent,1)
{
	init_tsText(sheadline, sbody);
}
void tsText::init_tsText(eString sheadline, eString sbody)
{
	addActionMap(&i_cursorActions->map);
	headline=new eLabel(this);
	headline->setText(sheadline);
	headline->setFont(eSkin::getActive()->queryFont("head"));
	body=new eLabel(this, RS_WRAP);
	body->setText(sbody);
}

int tsText::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		headline->move(ePoint(0, 0));
		headline->resize(eSize(size.width(), 40));
		body->move(ePoint(0, 40));
		body->resize(eSize(size.width(), size.height()-40));
		return 1;
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->ok)
		{
			close(0);
			return 1;
		}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

tsScan::tsScan(eWidget *parent, eString sattext)
	:eWidget(parent, 1), timer(eApp)
{
	init_tsScan(sattext);
}
void tsScan::init_tsScan(eString sattext)
{
	services_scanned = CreateSkinnedLabel("services_scanned");

	transponder_scanned = CreateSkinnedLabel("transponder_scanned");

	timeleft = CreateSkinnedLabel("time_left");

	service_name = CreateSkinnedLabel("service_name");

	service_provider = CreateSkinnedLabel("service_provider");

	transponder_data = CreateSkinnedLabel("transponder_data");

	progress = CreateSkinnedProgress("scan_progress");

	status = CreateSkinnedLabel("state");

	BuildSkin("tsScan");

	if ( sattext )
	{
		eString text = _("scanning...");
		text.erase( text.size()-3 );
		text += sattext;
		status->setText(text);
	}

	CONNECT(eDVB::getInstance()->eventOccured, tsScan::dvbEvent);
	CONNECT(eDVB::getInstance()->stateChanged, tsScan::dvbState);
	CONNECT(timer.timeout, tsScan::updateTime);
	CONNECT(eDVB::getInstance()->settings->getTransponders()->service_found, tsScan::serviceFound);
	CONNECT(eDVB::getInstance()->settings->getTransponders()->transponder_added, tsScan::addedTransponder);
}

int tsScan::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
	{
		scantime=0;
		eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
		if (!sapi)
		{
			eWarning("no scan active");
			close(1);
		} else
			sapi->start();
		break;
	}
	case eWidgetEvent::wantClose:
		if ( event.parameter == 1 ) // global Cancel
		{
			eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
			if ( sapi )
				sapi->abort();
			eWidgetEvent ev = event;
			ev.parameter=2;
			eFrontend::getInstance()->abortTune();
			return eWidget::eventHandler(ev);
		}
		eFrontend::getInstance()->abortTune();
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void tsScan::updateTime()
{
		scantime++;
		if ( tpScanned )
		{
			int sek = (int) (( (double) scantime / tpScanned) * tpLeft);
			if (sek > 59)
				timeleft->setText(eString().sprintf(_("%02i minutes and %02i seconds left"), sek / 60, sek % 60));
			else
				timeleft->setText(eString().sprintf(_("%02i seconds left"), sek ));
		}
}

void tsScan::serviceFound(const eServiceReferenceDVB &service, bool newService)
{
	servicesScanned++;
	
	services_scanned->setText(eString().sprintf("%i", servicesScanned));

	eServiceDVB *s=eDVB::getInstance()->settings->getTransponders()->searchService(service);
	service_name->setText(s->service_name);
	service_provider->setText(s->service_provider);
	
	if (newService)
		switch(s->service_type)
		{
			case 4:	// NVOD reference service
			case 1:	// digital television service
				newTVServices++;
				break;
			case 2:	// digital radio service
				newRadioServices++;
				break;
			case 3:	// teletext service
				break;
			case 5:	// NVOD time shifted service
				break;
			case 6:	// mosaic service
				break;
			default: // data
				newDataServices++;
			break;
		}
	else
		only_new_services=false;
}

void tsScan::addedTransponder( eTransponder* )
{
	newTransponders++;
	// hier landen wir jedesmal, wenn ein NEUER Transponder gefunden wurde...
}

void tsScan::dvbEvent(const eDVBEvent &event)
{
	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	
	int perc;

	switch (event.type)
	{
	case eDVBScanEvent::eventTunedIn:
		if ( !timer.isActive() )
			timer.start(1000);
		break;
	case eDVBScanEvent::eventScanBegin:
			tpLeft = sapi->getknownTransponderSize();
			progress->setPerc(0);
			tpScanned = newTVServices = newRadioServices = newDataServices = servicesScanned = newTransponders = 0;
			only_new_services=true;
		break;
	case eDVBScanEvent::eventScanTPadded:
			tpLeft++;
			perc=(int) ( ( 100.00 / (tpLeft+tpScanned) ) * tpScanned );
			progress->setPerc(perc);
		break;
	case eDVBScanEvent::eventScanTuneBegin:
		if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		{
			transponder_data->setText( eString().sprintf("%d MHz / %d ksyms / %s",
				event.transponder->satellite.frequency / 1000,
				event.transponder->satellite.symbol_rate / 1000,
				event.transponder->satellite.polarisation?_("Vertical"):"Horizontal") );
		}
		else if( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feCable )
		{
			transponder_data->setText( eString().sprintf("%d MHz / %d ksyms",
				event.transponder->cable.frequency / 1000,
				event.transponder->cable.symbol_rate / 1000));
		}
		else 
		{
			transponder_data->setText( eString().sprintf("%d KHz",
				event.transponder->terrestrial.centre_frequency/1000));
		}
		break;
	case eDVBScanEvent::eventScanNext:
			tpLeft--;
			tpScanned++;
			transponder_scanned->setText(eString().sprintf("%d", tpScanned));
			perc=(int) ( ( 100.00 / (tpLeft+tpScanned) ) * tpScanned );
			progress->setPerc(perc);
		break;
	case eDVBScanEvent::eventScanCompleted:
			timer.stop();
			close(0);
		break;
	default:
		break;
	}
}

void tsScan::dvbState(const eDVBState &state)
{
}

eListBoxEntrySat::eListBoxEntrySat( eListBox<eListBoxEntrySat> *lb, tpPacket *sat )
	:eListBoxEntryText( (eListBox<eListBoxEntryText>*) lb, sat->name, sat )
	,statePara(0), state(stateNotScan)
{
}

void eListBoxEntrySat::invalidate()
{
	if ( statePara )
	{
		statePara->destroy();
		statePara=0;
	}
}

const eString& eListBoxEntrySat::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )
{
	bool b;

	if ( (b = (state == 2)) )
		state = 0;

	eListBoxEntryText::redraw( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );

	eRect right = rect;
	right.setLeft( rect.right() - right.width()/3 );

	if (!statePara)
	{
		statePara = new eTextPara( eRect( 0, 0, right.width(), right.height() ) );
		statePara->setFont( font );
		statePara->renderString( this->state == stateScanFree ?
			_("[only free]") : this->state == stateScan ?
			_("[all]") : _("[nothing]") );
		statePara->realign( eTextPara::dirCenter );
	}
	rc->clip(right);
	rc->renderPara(*statePara, ePoint( right.left(), rect.top() ) );
	rc->clippop();

	return text;
}

tsMultiSatScan::tsMultiSatScan(eWidget *parent)
	:eWidget(parent)
{
	init_tsMultiSatScan();
}
void tsMultiSatScan::init_tsMultiSatScan()
{
	CONNECT( CreateSkinnedButton("start")->selected, eWidget::accept );

	satellites = new eListBox<eListBoxEntrySat>(this);
	satellites->setName("satellites");
	satellites->setFlags(eListBoxBase::flagLostFocusOnFirst|eListBoxBase::flagLostFocusOnLast);

	BuildSkin("tsMultiSat");

	int err;

	if(	(err = eTransponderList::getInstance()->reloadNetworks()) )
		eFatal("couldn't load Networks... \nplease check satellites.xml");

	std::map<int,eSatellite*> sats;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			sats[s->getOrbitalPosition()]=s;

	for ( std::list<tpPacket>::iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
		if ( sats.find(i->orbital_position) != sats.end() )
			new eListBoxEntrySat(satellites, &*i );

	CONNECT( satellites->selected, tsMultiSatScan::entrySelected );
}

struct copyNetwork
{
	std::list<scanEntry> &dest;

	copyNetwork(std::list<scanEntry> &dest)
		:dest(dest)
	{
	}

	bool operator()(eListBoxEntrySat& s)
	{
		if (s.state != eListBoxEntrySat::stateNotScan)
		{
			scanEntry e;
			e.onlyFree = s.state == eListBoxEntrySat::stateScanFree;
			e.packet = s.getTransponders();
			dest.push_back(e);
		}
		return 0;
	}
};

void tsMultiSatScan::getSatsToScan( std::list<scanEntry> &target )
{
	satellites->forEachEntry( copyNetwork(target) );
}

void tsMultiSatScan::entrySelected( eListBoxEntrySat * e )
{
	if ( e )
	{
		if ( e->state < eListBoxEntrySat::stateScanFree )
			++e->state;
		else
			e->state = eListBoxEntrySat::stateNotScan;
		// set new text...
		e->invalidate();
		// force redraw
		satellites->invalidateCurrent();
	}
	else
		close(1);
}

TransponderScan::TransponderScan( eWidget *LCDTitle, eWidget *LCDElement, tState init)
	:eWindow(0), current(0)
#ifndef DISABLE_LCD
	,LCDElement(LCDElement), LCDTitle(LCDTitle)
#endif
	,closeTimer(eApp), last_orbital_pos(0), remove_new_flags(false), stateInitial(init)
{
	init_TransponderScan();
}
void TransponderScan::init_TransponderScan()
{
	addActionMap(&i_cursorActions->map);
	setText(_("Transponder Scan"));
	cmove(ePoint(130, 110));
	cresize(eSize(460, 400));

	statusbar=new eStatusBar(this);
	statusbar->loadDeco();
	statusbar->move(ePoint(0, getClientSize().height()-50) );
	statusbar->resize( eSize( getClientSize().width(), 50 ) );
	CONNECT( closeTimer.timeout, TransponderScan::Close );
}

void TransponderScan::Close()
{
	close(ret);
}

TransponderScan::~TransponderScan()
{
}

void showScanPic()
{
	FILE *f = fopen(CONFIGDIR "/enigma/pictures/scan.mvi", "r");
	if ( f )
	{
		fclose(f);
		Decoder::displayIFrameFromFile(CONFIGDIR "/enigma/pictures/scan.mvi" );
	}
	else 
	{
		FILE *f = fopen(TUXBOXDATADIR "/enigma/pictures/scan.mvi", "r");
		if ( f )
		{
			fclose(f);
			Decoder::displayIFrameFromFile(TUXBOXDATADIR "/enigma/pictures/scan.mvi" );
		}
	}
}

int TransponderScan::Exec()
{
	tState state = stateInitial;

	eSize size=getClientSize()-eSize(0,30);

	eString text;

	show();

	eTransponder oldTp(*eDVB::getInstance()->settings->getTransponders());

	while (state != stateEnd)
	{
		// abort running PMT Scan ( onlyFree )
		eTransponderList::getInstance()->leaveTransponder(0);

		switch (state)
		{
		case stateMenu:
		{
			tsSelectType select(this);
#ifndef DISABLE_LCD
			select.setLCD( LCDTitle, LCDElement);
#endif
			current = &select;
			select.show();
			state = (tState) select.exec();
			current=0;
			select.hide();
			break;
		}
		case stateMulti:
		{
			tsMultiSatScan scan(this);
#ifndef DISABLE_LCD
			scan.setLCD( LCDTitle, LCDElement);
#endif
			scan.show();
			current = &scan;
			switch (scan.exec())
			{
			case 0:
				state=stateMultiScan;
				scan.getSatsToScan( toScan );
				toScan.sort();
				break;
			case 1:
				if ( stateInitial == stateMenu )
					state=stateMenu;
				else
					state=stateEnd;
				break;
			}
			scan.hide();
			current=0;
			break;
		}
		case stateManual:
		{
			Decoder::Flush();
			showScanPic();
			Decoder::locked=1;
			if ( !service )
			{
				service = eServiceInterface::getInstance()->service;
				// must stop running TS Playback ( demux source memory )
				if ( service && service.path && service.type == eServiceReference::idDVB )
					eServiceInterface::getInstance()->stop();
			}

			eTransponder transponder(*eDVB::getInstance()->settings->getTransponders());
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

			if ( oldTp.isValid() )
				transponder=oldTp;
			else if (sapi && sapi->transponder)
				transponder=*sapi->transponder;
			else
				switch (eSystemInfo::getInstance()->getFEType())
				{
				case eSystemInfo::feCable:
					transponder.setCable(402000, 6900000, 0, 3, 0);	// some cable transponder
					break;
				case eSystemInfo::feSatellite:
					transponder.setSatellite(12551500, 22000000, eFrontend::polVert, 4, 0, 0);	// some astra transponder
					break;
				case eSystemInfo::feTerrestrial:
					// most is AUTO
					transponder.setTerrestrial(522000000, 0, 1, 4, 5, 5, 4, 3, 2);
				default:
					break;
				}

			eDVB::getInstance()->setMode(eDVB::controllerScan);        

			eSize s = statusbar->getSize();
			ePoint pos = statusbar->getPosition();
			statusbar->hide();
			statusbar->resize( eSize( s.width(), s.height()-20 ) );
			statusbar->move( ePoint( pos.x(), pos.y()+20) );
			statusbar->show();
#ifndef DISABLE_LCD
			tsManual manual_scan(this, transponder, LCDTitle, LCDElement);
#else
			tsManual manual_scan(this, transponder);
#endif
			manual_scan.show();
			current = &manual_scan;
			switch (manual_scan.exec())
			{
			case 0:
				state=stateScan;
				break;
			case 1:
				if ( stateInitial == stateMenu )
					state=stateMenu;
				else
					state=stateEnd;
				break;
			}
			manual_scan.hide();
			statusbar->hide();
			statusbar->resize( s );
			statusbar->move( pos );
			statusbar->show();
			current=0;
			oldTp=manual_scan.getTransponder();
			break;
		}
		case stateAutomatic:
		{
			Decoder::Flush();
			showScanPic();
			Decoder::locked=1;

			if ( !service )
			{
				service = eServiceInterface::getInstance()->service;
				// must stop running TS Playback ( demux source memory )
				if ( service && service.path && service.type == eServiceReference::idDVB )
					eServiceInterface::getInstance()->stop();
			}

			tsAutomatic automatic_scan(this);
#ifndef DISABLE_LCD
			automatic_scan.setLCD( LCDTitle, LCDElement);
#endif
			automatic_scan.show();
			automatic_scan.openNetworkCombo();
			current = &automatic_scan;

			eDVB::getInstance()->setMode(eDVB::controllerScan);

			switch (automatic_scan.exec())
			{
			case 0:
				remove_new_flags=true;
				state=stateScan;
				break;
			default:
			case 1:
				if ( stateInitial == stateMenu )
					state=stateMenu;
				else
					state=stateEnd;
				break;
			}
			automatic_scan.hide();
			current=0;
			break;
		}
		case stateMultiScan:
		{
			int newTransponders,
					newTVServices,
					newRadioServices,
					newDataServices,
					tpScanned,
					servicesScanned,
					satScanned;

			newTransponders = newTVServices = newRadioServices =
			newDataServices = tpScanned = servicesScanned = satScanned = 0;

			if (!toScan.size())
			{
				eWarning("no satellites selected");
				state = stateEnd;
				break;
			}

			Decoder::Flush();
			showScanPic();
			Decoder::locked=1;

			if ( !service )
			{
				service = eServiceInterface::getInstance()->service;
				// must stop running TS Playback ( demux source memory )
				if ( service && service.path && service.type == eServiceReference::idDVB )
					eServiceInterface::getInstance()->stop();
			}

			while ( toScan.size() )
			{
				eDVB::getInstance()->setMode(eDVB::controllerService);
				eDVB::getInstance()->setMode(eDVB::controllerScan);
				// add transponder to scan api
				eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
				if (!sapi)
				{
					eWarning("no scan active");
					state = stateEnd;
					break;
				}
				else
				{
					tpPacket *pkt=toScan.front().packet;

					int snocircular=0;
					eConfig::getInstance()->getKey("/elitedvb/DVB/config/nocircular",snocircular);

					eLNB *lnb=0;
					if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
					{
						eSatellite *sat = eTransponderList::getInstance()->findSatellite( pkt->orbital_position );
						if ( sat )
							lnb = sat->getLNB();
						last_orbital_pos = pkt->orbital_position;
					}

					int cnt=0;
					for (std::list<eTransponder>::iterator i(pkt->possibleTransponders.begin()); i != pkt->possibleTransponders.end(); ++i)
					{
						if(snocircular)
							i->satellite.polarisation&=1;   // CEDR

						if ( lnb && !i->satellite.useable(lnb))
							continue;

						cnt++;
					}

					if (!cnt)  // no transponders to scan
					{
						toScan.erase(toScan.begin());
						++satScanned;
						continue;
					}

					eString str = ' '+toScan.front().packet->name;
					str += eString().sprintf("        (%d/%d)", satScanned+1, toScan.size()+satScanned);

					statusbar->setText(_("Waiting for tuner lock... please wait"));
					tsTryLock t(this, pkt, str);
					current = &t;
					t.show();
					int ret = t.exec();
					t.hide();
					current=0;
					switch(ret)
					{
						case -1:  // user abort
							toScan.clear();
							continue;
						case -2:  // no lock on this satellite
							toScan.erase(toScan.begin());
							continue;
					}

					for (std::list<eTransponder>::iterator i(pkt->possibleTransponders.begin()); i != pkt->possibleTransponders.end(); ++i)
					{
						if ( lnb && !i->satellite.useable(lnb))
							continue;
						sapi->addTransponder(*i);
					}

					// scanflags auswerten
					sapi->setUseONIT(pkt->scanflags & 4);
					sapi->setUseBAT(pkt->scanflags & 2);
					sapi->setNetworkSearch(pkt->scanflags & 1);
					sapi->setOnlyFree(toScan.front().onlyFree);

					// macht nur Probleme...bzw dauert recht lang...
					sapi->setSkipOtherOrbitalPositions(1);
//					sapi->setClearList(1);
					sapi->setNoCircularPolarization(snocircular);
				}

				eString str = ' '+toScan.front().packet->name;
				str += eString().sprintf("        (%d/%d)", satScanned+1, toScan.size()+satScanned);

				if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
					eTransponderList::getInstance()->removeNewFlags(sapi->getOrbitalPosition());
				else
					eTransponderList::getInstance()->removeNewFlags(-1);

				tsScan scan(this, str );
				current = &scan;
#ifndef DISABLE_LCD
				scan.setLCD( LCDTitle, LCDElement);
#endif
				scan.move(ePoint(0, 0));
				scan.resize(size);

				scan.show();
				statusbar->setText(_("Scan is in progress... please wait"));
				int ret = scan.exec();
				current=0;
				scan.hide();

				newTransponders += scan.newTransponders;
				newTVServices += scan.newTVServices;
				newRadioServices += scan.newRadioServices;
				newDataServices += scan.newDataServices;
				tpScanned += scan.tpScanned;
				servicesScanned += scan.servicesScanned;

				if ( scan.only_new_services )
				{
					if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
						eTransponderList::getInstance()->removeNewFlags(sapi->getOrbitalPosition());
					else
						eTransponderList::getInstance()->removeNewFlags(-1);
					eDVB::getInstance()->settings->saveServices();
				}

				toScan.erase(toScan.begin());
				++satScanned;
				if ( ret == 2 ) // user aborted
					toScan.clear();
				if ( newTVServices || newRadioServices )
					this->ret=0;
				else
					this->ret=1;
			} 

			text.sprintf(_("The transponder scan has finished and found \n   %i new Transponders,\n   %i new TV Services,\n   %i new Radio Services and\n   %i new Data Services.\n%i Transponders with %i Services scanned."),
				newTransponders, newTVServices,
				newRadioServices, newDataServices,
				tpScanned, servicesScanned );

			state=stateDone;
			break;
		}
		case stateScan:
		{
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
			{
				eDVBScanController *sapi = eDVB::getInstance()->getScanAPI();
				if ( sapi )
					last_orbital_pos = sapi->getOrbitalPosition();
				if ( !service )
				{
					service = eServiceInterface::getInstance()->service;
					// must stop running TS Playback ( demux source memory )
					if ( service && service.path && service.type == eServiceReference::idDVB )
						eServiceInterface::getInstance()->stop();
				}
				if ( remove_new_flags )
					eTransponderList::getInstance()->removeNewFlags(last_orbital_pos);
			}
			else if ( remove_new_flags )
				eTransponderList::getInstance()->removeNewFlags(-1);

			tsScan scan(this);
			current = &scan;
#ifndef DISABLE_LCD
			scan.setLCD( LCDTitle, LCDElement);
#endif
			scan.move(ePoint(0, 0));
			scan.resize(size);
			
			scan.show();
			statusbar->setText(_("Scan is in progress... please wait"));
			scan.exec();
			current=0;
			scan.hide();

			if ( scan.only_new_services && remove_new_flags )
			{
				if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
					eTransponderList::getInstance()->removeNewFlags(last_orbital_pos);
				else
					eTransponderList::getInstance()->removeNewFlags(-1);
				remove_new_flags=false;

				eDVB::getInstance()->settings->saveServices();
			}

			text.sprintf(_("The transponder scan has finished and found \n   %i new Transponders,\n   %i new TV Services,\n   %i new Radio Services and\n   %i new Data Services.\n%i Transponders with %i Services scanned."), scan.newTransponders, scan.newTVServices, scan.newRadioServices, scan.newDataServices, scan.tpScanned, scan.servicesScanned );

			if ( scan.newTVServices || scan.newRadioServices )
				this->ret = 0;
			else
				this->ret = 1;

			state=stateDone;
			break;
		}
		case stateDone:
		{
			tsText finish(_("Done."), text, this);
			current = &finish;
#ifndef DISABLE_LCD
			finish.setLCD( LCDTitle, LCDElement);
#endif
			finish.move(ePoint(0, 0));
			finish.resize(size);
			finish.show();
			statusbar->setText(_("Scan is in finished, press ok to close window"));
			finish.exec();
			current=0;
			finish.hide();
			if ( stateInitial == stateManual ||
				eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
			{
				eWindow::globalCancel( eWindow::ON );
				int res = eMessageBox::ShowBox(eString().sprintf(_("Do you want\nto scan another\n%s?"),stateInitial==stateManual?_("Transponder"):_("Satellite")),
					_("Scan finished"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
					eMessageBox::btYes );
				switch ( res )
				{
					case -1:
					case eMessageBox::btNo:
						state=stateEnd;
						break;
					default:
						state=stateInitial;
				}
				eWindow::globalCancel( eWindow::OFF );
				eDVB::getInstance()->setMode(eDVB::controllerService);
				break;
			}
		}
		default:
			state=stateEnd;
			break;
		}
	}
	hide();
	eDVB::getInstance()->setMode(eDVB::controllerService);
	Decoder::clearScreen();

	return ret;
}

int TransponderScan::eventHandler( const eWidgetEvent &event )
{
	switch (event.type)
	{
		case eWidgetEvent::childChangedHelpText:
			if (focus)
				statusbar->setText(focus->getHelpText());
			break;
		case eWidgetEvent::execBegin:
			eWindow::globalCancel( eWindow::OFF );
			ret = Exec();
			closeTimer.start(0,true);
			break;
		case eWidgetEvent::execDone:
			if ( Decoder::locked )
			{
				Decoder::locked=0;
				Decoder::Flush();
				if ( service /*&& ( !last_orbital_pos || ((((eServiceReferenceDVB&)service).getDVBNamespace().get() & 0xFFFF0000) >> 16 ) == last_orbital_pos )*/ )
				{
					eFrontend::getInstance()->savePower();
					eServiceInterface::getInstance()->service=eServiceReference();
					eZapMain::getInstance()->playService(service, eZapMain::psDontAdd|eZapMain::psSetMode );
				}
			}
			eWindow::globalCancel( eWindow::ON );
			break;
		case eWidgetEvent::evtAction:
			if ( event.action == &i_cursorActions->cancel && current )  // don't ask !
			{
				if ( focus && focus != this && focus->eventHandler(event) )
					;
				else if ( current && focus != this )
					current->close(1);
				return 1;
			}
		default:
			break;
	}
	return eWindow::eventHandler( event );
}

class testWidget : public eWidget
{
	int eventHandler( const eWidgetEvent &evt )
	{
		if ( evt.type == eWidgetEvent::evtAction )
		{
			close(0);
			return 1;
		}
		return eWidget::eventHandler(evt);
	}
public:
	testWidget()
		:eWidget(0,1)
	{
		addActionMap(&i_cursorActions->map);
		addActionMap(&i_listActions->map);
		addActionMap(&i_focusActions->map);
		addActionMap(&i_numberActions->map);
	}
};

ManualPIDWindow::ManualPIDWindow(eTransponder *tp, const eServiceReferenceDVB &ref )
	:eWindow(0), hex(false), transponder(*eDVB::getInstance()->settings->getTransponders())
	,service(ref), pat(0), nit(0)
{
	init_ManualPIDWindow(tp,ref);
}

void ManualPIDWindow::init_ManualPIDWindow(eTransponder *tp, const eServiceReferenceDVB &ref )
{
	if ( tp )
		transponder = *tp;

	name = CreateSkinnedTextInputField("name",0,0, "lname");
	name->setEditHelpText(_("enter service name"));

	provider =CreateSkinnedTextInputField("provider",0,0, "lprovider");
	provider->setEditHelpText(_("enter provider name"));

	vpid =CreateSkinnedTextInputField("vpid",0,0, "lvpid");
	vpid->setEditHelpText(_("enter video pid"));

	pcrpid =CreateSkinnedTextInputField("pcrpid",0,0, "lpcrpid");
	pcrpid->setEditHelpText(_("enter pcr pid (in most case the same pid as the video pid)"));

	apid =CreateSkinnedTextInputField("apid",0,0, "lapid");
	apid->setEditHelpText(_("enter audio pid"));

	isAC3Pid = CreateSkinnedCheckbox("isAC3Pid");

	tpid =CreateSkinnedTextInputField("tpid",0,0, "ltpid");
	tpid->setEditHelpText(_("enter (video) text pid"));

	sid =CreateSkinnedTextInputField("sid",0,0, "lsid");
	sid->setEditHelpText(_("enter service id"));

	tsid =CreateSkinnedTextInputField("tsid",0,0, "ltsid");
	tsid->setEditHelpText(_("enter tsid (transport stream id)"));

	onid =CreateSkinnedTextInputField("onid",0,0, "lonid");
	onid->setEditHelpText(_("enter onid (original network id)"));

	cNoDVB = CreateSkinnedCheckbox("cNoDVB");
	cUseSDT = CreateSkinnedCheckbox("cUseSDT");
	cHoldName = CreateSkinnedCheckbox("cHoldName");

	bHexDec = CreateSkinnedButton("bHexDec");
	CONNECT(bHexDec->selected, ManualPIDWindow::hexdec);

	CONNECT(CreateSkinnedButton("bReadNIT")->selected, ManualPIDWindow::startNIT);

	CONNECT(CreateSkinnedButton("bSetPIDs")->selected, ManualPIDWindow::setPIDs);

	CONNECT(CreateSkinnedButton("bStore")->selected, ManualPIDWindow::store);

	BuildSkin("ManualPIDWindow");
	int bla=1;
	eConfig::getInstance()->getKey("/elitedvb/scan/manualpids/hexdec", bla);
	while (bla--)
		hexdec();

	if ( service )
	{
		if ( !transponder.isValid() )
		{
			eTransponder *tmp =
				eDVB::getInstance()->settings->getTransponders()->searchTS(
					service.getDVBNamespace(), service.getTransportStreamID(), service.getOriginalNetworkID());
			if ( tmp )
				transponder = *tmp;
		}
		eService *sp=eServiceInterface::getInstance()->addRef(service);
		if ( sp )
		{
			if ( sp->dvb )  // get cached pids when avail
			{
				int val=-1;
				char tmp[6];
				sprintf(tmp, hex?"%04x":"%d", sp->dvb->service_id.get());
				sid->setText(tmp);
				val=sp->dvb->get(eServiceDVB::cVPID);
				if ( val != -1 )
				{
					sprintf(tmp, hex?"%04x":"%d", val);
					vpid->setText(tmp);
				}
				else
					vpid->setText("0");
				val=sp->dvb->get(eServiceDVB::cAPID);
				if ( val != -1 )
				{
					sprintf(tmp, hex?"%04x":"%d", val);
					apid->setText(tmp);
				}
				else
				{
					val=sp->dvb->get(eServiceDVB::cAC3PID);
					if ( val != -1 )
					{
						isAC3Pid->setCheck(1);
						sprintf(tmp, hex?"%04x":"%d", val);
						apid->setText(tmp);
					}
					else
						apid->setText("0");
				}
				val=sp->dvb->get(eServiceDVB::cPCRPID);
				if ( val != -1 )
				{
					sprintf(tmp, hex?"%04x":"%d", val);
					pcrpid->setText(tmp);
				}
				else
					pcrpid->setText("0");
				val=sp->dvb->get(eServiceDVB::cTPID);
				if ( val != -1 )
				{
					sprintf(tmp, hex?"%04x":"%d", val);
					tpid->setText(tmp);
				}
			}
			cUseSDT->setCheck( !(sp->dvb->dxflags & eServiceDVB::dxNoSDT) );
			cHoldName->setCheck( sp->dvb->dxflags & eServiceDVB::dxHoldName );
			cNoDVB->setCheck( sp->dvb->dxflags & eServiceDVB::dxNoDVB );
			eServiceInterface::getInstance()->removeRef(service);
		}
	}
	else
	{
		cNoDVB->setCheck(1);
		cHoldName->setCheck(1);
		name->setText("unnamed service");
		eString service_provider;
		if ( transponder.satellite.isValid() )
		{
			service_provider.sprintf("%d %d.%d°%c",
				transponder.satellite.frequency/1000,
				abs(transponder.satellite.orbital_position)/10,
				abs(transponder.satellite.orbital_position)%10,
				transponder.satellite.orbital_position > 0 ? 'E' : 'W');
		}
		else if ( transponder.cable.isValid() )
			service_provider.sprintf("%d", transponder.cable.frequency/1000);
		else if ( transponder.terrestrial.isValid() )
			service_provider.sprintf("%d", transponder.terrestrial.centre_frequency/1000);
		provider->setText(service_provider);
	}
	char tmp[6];
	sprintf(tmp, hex?"%04x":"%d", transponder.transport_stream_id.get());
	tsid->setText(tmp);
	sprintf(tmp, hex?"%04x":"%d", transponder.original_network_id.get());
	onid->setText(tmp);
	eWindow::globalCancel(eWindow::ON);
}
ManualPIDWindow::~ManualPIDWindow()
{
	delete nit;
	delete pat;
}

void ManualPIDWindow::startNIT()
{
	delete nit;
	nit=0;
	delete pat;
	pat = new PAT();
	CONNECT(pat->tableReady, ManualPIDWindow::gotNIT);
	pat->start();
}

void ManualPIDWindow::gotNIT(int err)
{
	int nitpid = 0x10;
	bool found=false;
	if (!nit) // pat scan to find nit pid
	{
		if (!err)
		{
			PATEntry *p = pat->searchService(0); // NIT PID
			if ( p )
				nitpid = p->program_map_PID;
		}
		nit = new NIT(nitpid);
		CONNECT(nit->tableReady, ManualPIDWindow::gotNIT);
		nit->start();
	}
	else
	{
		if ( err )
		{
			eMessageBox::ShowBox(_("Reading NIT failed... this transponder have no NIT..\nso you can use random values for tsid and onid or set both to 0"),
				_("Error"), eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK);
		}
		else
		{
			for (ePtrList<NITEntry>::iterator ne(nit->entries); ne != nit->entries.end(); ++ne)
			{
				eTransponder tmp(*eDVB::getInstance()->settings->getTransponders());
				ePtrList<Descriptor>::iterator de(ne->transport_descriptor);
				for (; de != ne->transport_descriptor.end(); ++de)
				{
					if ( transponder.satellite.isValid() && de->Tag() == DESCR_SAT_DEL_SYS )
					{
						tmp.setSatellite( (SatelliteDeliverySystemDescriptor*)*de );
						break;
					}
					else if ( transponder.cable.isValid() && de->Tag() == DESCR_CABLE_DEL_SYS )
					{
						tmp.setCable( (CableDeliverySystemDescriptor*)*de );
						break;
					}
					else if ( transponder.terrestrial.isValid() && de->Tag() == DESCR_TERR_DEL_SYS )
					{
						tmp.setTerrestrial( (TerrestrialDeliverySystemDescriptor*)*de );
						break;
					}
				}
				if (de != ne->transport_descriptor.end())
				{
					if ( tmp.satellite.isValid() )
					{
						if ( transponder.satellite == tmp.satellite )
							found=true;
						else if ( transponder.cable == tmp.cable )
							found=true;
						else if ( transponder.terrestrial == tmp.terrestrial )
							found=true;
						if ( found )
						{
							char tmp[6];
							sprintf(tmp, hex?"%04x":"%d", ne->transport_stream_id);
							tsid->setText( tmp );
							sprintf(tmp, hex?"%04x":"%d", ne->original_network_id);
							onid->setText( tmp );
							return;
						}
					}
				}
			}
		}
		if ( !found )
		{
			eMessageBox::ShowBox(_("No NIT Entry for current transponder values found...\nso you can use random values for tsid and onid or set both to 0"),
				_("Error"), eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK);
		}
	}
}

void ManualPIDWindow::setPIDs()
{
	Decoder::locked=0;
	hide();
	int val=0;
	sscanf( vpid->getText().c_str(), hex?"%x":"%d", &val);
	if ( val != 0 )
	{
		Decoder::parms.vpid = val;
		val=0;
	}
	sscanf( apid->getText().c_str(), hex?"%x":"%d", &val);
	if ( val != 0 )
	{
		Decoder::parms.apid = val;
		val=0;
	}
	sscanf( tpid->getText().c_str(), hex?"%x":"%d", &val);
	if ( val != 0 )
	{
		Decoder::parms.tpid = val;
		val=0;
	}
	sscanf( pcrpid->getText().c_str(), hex?"%x":"%d", &val);
	if ( val != 0 )
		Decoder::parms.pcrpid = val;
	Decoder::parms.audio_type =
		isAC3Pid->isChecked() ? DECODE_AUDIO_AC3 : DECODE_AUDIO_MPEG;
	Decoder::Set();
	testWidget w;
	w.show();
	w.exec();
	w.hide();
	show();
	Decoder::Flush();
	Decoder::locked=1;
	showScanPic();
}

void ManualPIDWindow::store()
{
	eDVBNamespace dvb_namespace;
	int tsid=0,
		onid=0;

	eTransponderList &tlist =
		*eTransponderList::getInstance();

	if ( service )
		tlist.removeService(service);
	else
		service.type = eServiceReference::idDVB;

	if ( transponder.isValid() )
	{
		if ( !tlist.countServices(
			transponder.dvb_namespace,
			transponder.transport_stream_id,
			transponder.original_network_id) )
			tlist.removeTransponder(transponder);
	}

	sscanf(this->tsid->getText().c_str(), hex?"%x":"%d", &tsid);
	sscanf(this->onid->getText().c_str(), hex?"%x":"%d", &onid);

		// build "namespace" to work around buggy satellites
	if (transponder.satellite.valid)
		dvb_namespace=eTransponder::buildNamespace(onid, tsid,
			transponder.satellite.orbital_position,
			transponder.satellite.frequency,
			transponder.satellite.polarisation);
	else if (transponder.cable.valid)
		dvb_namespace=eTransponder::buildNamespace(onid, tsid,
			0xFFFF,
			transponder.cable.frequency,
			0);
	else if (transponder.terrestrial.valid)
		dvb_namespace=eTransponder::buildNamespace(onid, tsid,
			0xEEEE,
			transponder.terrestrial.centre_frequency/1000, // centre_freq is in hz
			0);

	transponder.dvb_namespace=dvb_namespace;

	eTransponder *tmp = tlist.searchTS(dvb_namespace, eTransportStreamID(tsid), eOriginalNetworkID(onid));

	if( tmp ) // we found a transponder with the same namespace/tsid/onid
	{
		bool do_abort=true;
		if ( tmp->satellite.isValid() && transponder.satellite.isValid() )
		{
			if ( tmp->satellite == transponder.satellite )
				do_abort=false;
		}
		else if ( tmp->cable.isValid() && transponder.cable.isValid() )
		{
			if ( tmp->cable == transponder.cable )
				do_abort=false;
		}
		else if ( tmp->terrestrial.isValid() && transponder.terrestrial.isValid() )
		{
			if ( tmp->cable == transponder.cable )
				do_abort=false;
		}
		if (do_abort)
		{
			eMessageBox::ShowBox(_("A transponder with the same tsid / onid but other frequency/pol/... already exist.. create a new transponder with the same data is no possible.. please do change tsid and/or onid"),
				_("Error"),
				eMessageBox::iconError|eMessageBox::btOK, eMessageBox::btOK);
			return;
		}
	}

	if ( !tmp )
	{
		// we must search transponder via freq pol usw..
		transponder.transport_stream_id = -1;
		transponder.original_network_id = -1;
		tmp = tlist.searchTransponder(transponder);
	}

	// ok we found the transponder, it seems to be valid
	// get Reference to the new Transponder
	eTransponder &real = tmp?*tmp:tlist.createTransponder(dvb_namespace, tsid, onid);

	transponder.transport_stream_id = real.transport_stream_id;
	transponder.original_network_id = real.original_network_id;

	// replace referenced transponder with new transponderdata
	if ( !tmp )  // use existing transponder
		real=transponder;  // save our frequency/sr/pol to transponderlist..

	real.state=eTransponder::stateOK;

	service.setTransportStreamID(real.transport_stream_id);
	service.setOriginalNetworkID(real.original_network_id);
	service.setDVBNamespace(real.dvb_namespace);

	int tmpval=0;
	sscanf(sid->getText().c_str(), hex?"%x":"%d", &tmpval);
	service.setServiceID(eServiceID(tmpval));

	bool newAdded=false;

	if ( vpid->getText() != hex ? "0000" : "0" )
		service.setServiceType(1);
	else if ( apid->getText() != hex ? "0000" : "0" )
		service.setServiceType(2);
	else
		service.setServiceType(100);

	eServiceDVB &s =
		tlist.createService( service, -1, &newAdded );

	if ( !newAdded )
	{
		eString service_name =  s.service_name ? s.service_name : eString("unnamed service");
		eString service_provider =  s.service_provider ? s.service_provider : eString("unnamed provider");

		int ret = eMessageBox::ShowBox(eString().sprintf(_("A service named '%s' with this sid/onid/tsid/namespace is already exist\n"
			"in provider '%s'.\nShould i use this service name and provider name?"), service_name.c_str(), service_provider.c_str() ),
			_("Service already exist"),
			eMessageBox::iconQuestion|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btYes);

		if ( ret == eMessageBox::btYes )  // change provider and service name...
			;
		else
		{
			// provider
			s.service_provider = provider->getText();
			// service name
			s.service_name = name->getText();
		}
	}
	else
	{
		// provider
		s.service_provider = provider->getText();
		// service name
		s.service_name = name->getText();
	}

	// reset cached pids
	s.clearCache();  

	// set service type
	s.service_type = service.getServiceType();

	tmpval=0;
	// video pid
	sscanf( vpid->getText().c_str(), hex?"%x":"%d", &tmpval);
	if (tmpval)
	{
		s.set(eServiceDVB::cVPID,tmpval);
		tmpval=0;
	}

	// audio / ac3 pid
	sscanf( apid->getText().c_str(), hex?"%x":"%d", &tmpval);
	if (tmpval)
	{
		s.set(isAC3Pid->isChecked()?eServiceDVB::cAC3PID:eServiceDVB::cAPID,tmpval);
		tmpval=0;
	}

	// text pid
	sscanf( tpid->getText().c_str(), hex?"%x":"%d", &tmpval);
	if (tmpval)
	{
		s.set(eServiceDVB::cTPID,tmpval);
		tmpval=0;
	}

	// pcr pid
	sscanf( pcrpid->getText().c_str(), hex?"%x":"%d", &tmpval);
	if (tmpval)
		s.set(eServiceDVB::cPCRPID,tmpval);

	// DX Flags
	s.dxflags=eServiceDVB::dxNewFound;
	if (cNoDVB->isChecked())
		s.dxflags |= eServiceDVB::dxNoDVB;
	if (cUseSDT->isChecked())
		s.dxflags |= eServiceDVB::dxNoSDT;
	if (cHoldName->isChecked())
		s.dxflags |= eServiceDVB::dxHoldName;

	eDVB &dvb = *eDVB::getInstance();
	dvb.settings->saveServices();
	dvb.settings->sortInChannels();
	dvb.settings->saveBouquets();

	close(0);
}

void ManualPIDWindow::hexdec()
{
	hex = !hex;
	const char *useablechars = "1234567890abcdefABCDEF";
	int maxchars=4;
	if (!hex)
	{
		eConfig::getInstance()->setKey("/elitedvb/scan/manualpids/hexdec", 2);
		useablechars = "1234567890";
		maxchars=5;
		bHexDec->setText("HEX");
		bHexDec->setHelpText(_("show/enter values as hexadecimal"));
	}
	else
	{
		eConfig::getInstance()->setKey("/elitedvb/scan/manualpids/hexdec", 1);
		bHexDec->setText("DEC");
		bHexDec->setHelpText(_("show/enter values as decimal"));
	}
	eTextInputField **p = ( &vpid < &sid ) ? &vpid : &sid;
	for (int i=0; i < 7; ++i, ++p)
	{
		int val=0;
		char tmp[6];
		(*p)->setMaxChars(maxchars);
		(*p)->setUseableChars(useablechars);
		sscanf((*p)->getText().c_str(), hex?"%d":"%x", &val);
		sprintf(tmp, hex?"%04x":"%d", val);
		(*p)->setText(tmp);
	}
}
