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
	:eWidget(parent,1)
{
	list=new eListBox<eListBoxEntryText>(this);
	list->setName("menu");
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsSelectType"))
		eFatal("skin load of \"tsSelectType\" failed");

	list->setFlags(eListBox<eListBoxEntryText>::flagShowEntryHelp);
	new eListBoxEntryText(list, _("Automatic Transponder Scan"), (void*)2, 0, _("open automatic transponder scan") );
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		new eListBoxEntryText(list, _("Automatic Multisat Scan"), (void*)3, 0, _("open automatic multisat transponder scan") );
	new eListBoxEntryText(list, _("manual scan.."), (void*)1, 0, _("open manual transponder scan") );
	CONNECT(list->selected, tsSelectType::selected);
}

void tsSelectType::selected(eListBoxEntryText *entry)
{
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
:eWidget(parent), transponder(transponder), updateTimer(eApp)
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

	c_useonit=new eCheckbox(this);
	c_useonit->setName("useonit");
	
	c_usebat=new eCheckbox(this);
	c_usebat->setName("usebat");
	
	c_onlyFree=new eCheckbox(this);
	c_onlyFree->setName("onlyFree");

	c_searchnit=new eCheckbox(this);
	c_searchnit->setName("searchnit");

	b_start=new eButton(this);
	b_start->setName("start");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsManual"))
		eFatal("skin load of \"tsManual\" failed");

	transponder_widget->load();
	transponder_widget->setTransponder(&transponder);

	CONNECT(b_start->selected, tsManual::start);
	CONNECT(transponder_widget->updated, tsManual::retune);
//	CONNECT(updateTimer.timeout, tsManual::update );
	setHelpID(62);
}

void tsManual::update()
{
	int status=eFrontend::getInstance()->Status();
	if (!(status & FE_HAS_LOCK))
	{
		if (!transponder_widget->getTransponder(&transponder))
			transponder.tune();
	}
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

void tsManual::retune()
{
	if (!transponder_widget->getTransponder(&transponder))
		transponder.tune();
}

int tsManual::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		updateTimer.start(1000);
		break;
	case eWidgetEvent::execDone:
		updateTimer.stop();
	default:
		return eWidget::eventHandler(event);
		break;
	}
	return 0;
}

tsTryLock::tsTryLock(eWidget *parent, tpPacket *packet, eString ttext)
	:eWidget(parent), ret(0), inProgress(0), packet(packet)
	,current_tp(packet->possibleTransponders.begin())	
{
	l_status=new eLabel(this, RS_WRAP);
	l_status->setName("lStatus");

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");

	b_abort=new eButton(this);
	b_abort->setName("bAbort");

	eSkin *skin=eSkin::getActive();

	if ( skin->build(this, "tsTryLock") )
		eFatal("skin load of \"tsTryLock\" failed");

	eLabel *l = (eLabel*)search("lNet");
	if (l)
	{
		eString text = l->getText();
		text.erase( text.size()-3 );
		text+=ttext;
		l->setText(text);
	}

	CONNECT(b_abort->selected, tsTryLock::reject);
	CONNECT(eDVB::getInstance()->eventOccured, tsTryLock::dvbEvent);
}

void tsTryLock::dvbEvent(const eDVBEvent &event)
{
	if (!inProgress)
		return;
	switch (event.type)
	{
		case eDVBScanEvent::eventTunedIn:
			inProgress=0;
			if (ret)
				close(ret);
			else if (event.err)
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
			inProgress=0;
			if ( e.parameter == 1 ) // globalCancel
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
	:eWidget(parent), ret(0), inProgress(0)
{
	eLabel* l = new eLabel(this);
	l->setName("lNet");
	l_network=new eComboBox(this, 3, l);
	l_network->setName("network");

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");
	
	l_status=new eLabel(this, RS_WRAP);
	l_status->setName("status");

	c_onlyFree = new eCheckbox(this,0);
	c_onlyFree->setName("onlyFree");
	c_onlyFree->hide();
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
	{
		int snocircular=0;
		eConfig::getInstance()->getKey("/elitedvb/DVB/config/nocircular",snocircular);
		c_nocircular=new eCheckbox(this,snocircular);
		c_nocircular->setName("nocircular");
		c_nocircular->hide();
	}
	else
		c_nocircular=0;

	b_start=new eButton(this);
	b_start->setName("start");
	b_start->hide();

	eSkin *skin=eSkin::getActive();

	eString tmp = "tsAutomatic";

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		tmp+="_sat";
	else
		tmp+="_cable";

	if (skin->build(this, tmp.c_str()))
		eFatal("skin load of \"%s\" failed", tmp.c_str());

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
		sapi->setClearList(1);

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
			eDebug("tsAutomatic eventScanCompleted");
			close(ret);
			break;            
		case eDVBScanEvent::eventTunedIn:
			inProgress=0;
			if (ret)
				close(ret);
			else if (event.err)
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

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			for ( std::list<tpPacket>::const_iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
				if ( ( i->orbital_position == s->getOrbitalPosition() ) || (eSystemInfo::getInstance()->getFEType() != eSystemInfo::feSatellite) )
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

int tsAutomatic::eventHandler(const eWidgetEvent &e)
{
	switch(e.type)
	{
		case eWidgetEvent::wantClose:
			if (b_start->isVisible() || !inProgress)
				break;
			else
				ret=e.parameter;
			return 1;
		default:
			break;
	}
	return eWidget::eventHandler(e);
}

tsText::tsText(eString sheadline, eString sbody, eWidget *parent)
	:eWidget(parent, 1)
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
		if (event.action == &i_cursorActions->ok ||
				event.action == &i_cursorActions->cancel)
			close(0);
		else
			break;
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

tsScan::tsScan(eWidget *parent, eString sattext)
	:eWidget(parent, 1), timer(eApp), ret(0)
{
	addActionMap(&i_cursorActions->map);

	services_scanned = new eLabel(this);
	services_scanned->setName("services_scanned");

	transponder_scanned = new eLabel (this);
	transponder_scanned->setName("transponder_scanned");

	timeleft = new eLabel(this);
	timeleft->setName("time_left");

	service_name = new eLabel(this);
	service_name->setName("service_name");

	service_provider = new eLabel(this);
	service_provider->setName("service_provider");

	transponder_data = new eLabel(this);
	transponder_data->setName("transponder_data");

	progress = new eProgress(this);
	progress->setName("scan_progress");

	status = new eLabel(this);
	status->setName("state");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsScan"))
		eFatal("skin load of \"tsScan\" failed");

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
	case eWidgetEvent::evtAction:
		if ( event.action == &i_cursorActions->cancel )
		{
			eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
			if ( sapi && sapi->abort() )
				ret=2;
			else
				close(2);
			return 1;
		}
		else
			break;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void tsScan::updateTime()
{
		scantime++;
		int sek = (int) (( (double) scantime / tpScanned) * tpLeft);
		if (sek > 59)
			timeleft->setText(eString().sprintf(_("%02i minutes and %02i seconds left"), sek / 60, sek % 60));
		else
			timeleft->setText(eString().sprintf(_("%02i seconds left"), sek ));
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
			close(ret);
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
	start = new eButton(this);
	start->setName("start");

	satellites = new eListBox<eListBoxEntrySat>(this);
	satellites->setName("satellites");
	satellites->setFlags(eListBoxBase::flagLostFocusOnFirst|eListBoxBase::flagLostFocusOnLast);

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsMultiSat"))
		eFatal("skin load of \"tsMultiSat\" failed");

	int err;

	if(	(err = eTransponderList::getInstance()->reloadNetworks()) )
		eFatal("couldn't load Networks... \nplease check satellites.xml");

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			for ( std::list<tpPacket>::iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
				if ( ( i->orbital_position == s->getOrbitalPosition() ) || (eSystemInfo::getInstance()->getFEType() != eSystemInfo::feSatellite) )
					new eListBoxEntrySat(satellites, &*i );

	CONNECT( satellites->selected, tsMultiSatScan::entrySelected );
	CONNECT( start->selected, eWidget::accept );
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

int tsMultiSatScan::eventHandler( const eWidgetEvent &e )
{
	switch (e.type)
	{
	case eWidgetEvent::execBegin:
		setFocus(satellites);
		break;
	default:
		return eWidget::eventHandler( e );
		break;
	}
	return 0;
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
	,closeTimer(eApp), last_orbital_pos(0), stateInitial(init)
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
					sapi->setClearList(1);
					sapi->setNoCircularPolarization(snocircular);
				}

				eString str = ' '+toScan.front().packet->name;
				str += eString().sprintf("        (%d/%d)", satScanned+1, toScan.size()+satScanned);

				tsScan scan(this, str );
#ifndef DISABLE_LCD
				scan.setLCD( LCDTitle, LCDElement);
#endif
				scan.move(ePoint(0, 0));
				scan.resize(size);

				scan.show();
				statusbar->setText(_("Scan is in progress... please wait"));
				int ret = scan.exec();
				scan.hide();

				newTransponders += scan.newTransponders;
				newTVServices += scan.newTVServices;
				newRadioServices += scan.newRadioServices;
				newDataServices += scan.newDataServices;
				tpScanned += scan.tpScanned;
				servicesScanned += scan.servicesScanned;

				toScan.erase(toScan.begin());
				++satScanned;
				if ( ret == 2 ) // user aborted
					toScan.clear();
				if ( newTVServices || newRadioServices )
					this->ret=0;
				else
					this->ret=1;
			} 

			text.sprintf(_("The transponder scan has finished and found \n   %i new Transponders,\n   %i new TV Services,\n   %i new Radio Services and\n   %i new Data Services.\n%i Transponders within %i Services scanned."),
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
			}

			tsScan scan(this);
#ifndef DISABLE_LCD
			scan.setLCD( LCDTitle, LCDElement);
#endif
			scan.move(ePoint(0, 0));
			scan.resize(size);
			
			scan.show();
			statusbar->setText(_("Scan is in progress... please wait"));
			scan.exec();
			scan.hide();

			text.sprintf(_("The transponder scan has finished and found \n   %i new Transponders,\n   %i new TV Services,\n   %i new Radio Services and\n   %i new Data Services.\n%i Transponders within %i Services scanned."), scan.newTransponders, scan.newTVServices, scan.newRadioServices, scan.newDataServices, scan.tpScanned, scan.servicesScanned );

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
#ifndef DISABLE_LCD
			finish.setLCD( LCDTitle, LCDElement);
#endif
			finish.move(ePoint(0, 0));
			finish.resize(size);
			finish.show();
			statusbar->setText(_("Scan is in finished, press ok to close window"));
			finish.exec();
			finish.hide();
			if ( stateInitial == stateManual ||
				eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
			{
				eMessageBox mb(eString().sprintf(_("Do you want\nto scan another\n%s?"),stateInitial==stateManual?_("Transponder"):_("Satellite")),
					_("Scan finished"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
					eMessageBox::btYes );
				mb.show();
				switch ( mb.exec() )
				{
					case -1:
					case eMessageBox::btNo:
						state=stateEnd;
						break;
					default:
						state=stateInitial;
				}
				mb.hide();
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
