#include <time.h>
#include <scan.h>
#include <enigma.h>

#include <lib/dvb/frontend.h>
#include <lib/dvb/si.h>
#include <lib/dvb/dvb.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ewindow.h>
#include <lib/gdi/font.h>
#include <lib/gui/eprogress.h>
#include <lib/dvb/edvb.h>
#include <lib/driver/rc.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/combobox.h>
#include <lib/gui/guiactions.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/dvbscan.h>
#include <lib/dvb/dvbservice.h>

#include <string>

tsSelectType::tsSelectType(eWidget *parent): eWidget(parent)
{
	list=new eListBox<eListBoxEntryText>(this);
	list->setName("menu");
	list->move(ePoint(100, 100));
	list->resize(eSize(100, 100));
	
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsSelectType"))
		eFatal("skin load of \"tsSelectType\" failed");

	new eListBoxEntryText(list, _("auto scan"), (void*)1);
	new eListBoxEntryText(list, _("manual scan.."), (void*)2);
	list->setCurrent( new eListBoxEntryText(list, _("abort"), (void*)0) );

	CONNECT(list->selected, tsSelectType::selected);
}

void tsSelectType::selected(eListBoxEntryText *entry)
{
	if (!entry)
		close(0);
	else
		close((int)entry->getKey());
}

tsManual::tsManual(eWidget *parent, const eTransponder &transponder, eWidget *LCDTitle, eWidget *LCDElement)
:eWidget(parent), transponder(transponder)
{
	setLCD(LCDTitle, LCDElement);
	int ft=0;
	switch (eFrontend::getInstance()->Type())
	{
	case eFrontend::feSatellite:
		ft=eTransponderWidget::deliverySatellite;
		break;
	case eFrontend::feCable:
		ft=eTransponderWidget::deliveryCable;
		break;
	default:
		ft=eTransponderWidget::deliverySatellite;
		break;
	}

	transponder_widget=new eTransponderWidget(this, 1, ft);
	transponder_widget->load();
	transponder_widget->setName("transponder");

	festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");

	c_useonit=new eCheckbox(this);
	c_useonit->setName("useonit");
	
	c_usebat=new eCheckbox(this);
	c_usebat->setName("usebat");
	
	c_clearlist=new eCheckbox(this);
	c_clearlist->setName("clearlist");

	c_searchnit=new eCheckbox(this);
	c_searchnit->setName("searchnit");

	b_start=new eButton(this);
	b_start->setName("start");

	b_abort=new eButton(this);
	b_abort->setName("abort");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsManual"))
		eFatal("skin load of \"tsManual\" failed");

	transponder_widget->setTransponder(&transponder);

	CONNECT(b_start->selected, tsManual::start);
	CONNECT(b_abort->selected, tsManual::abort);
	CONNECT(transponder_widget->updated, tsManual::retune);

	retune();
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
		sapi->setClearList(c_clearlist->isChecked());
			
		close(0);
	}
}

void tsManual::abort()
{
	close(1);
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
		retune();
		break;
	default:
		break;
	}
	return 0;
}

tsAutomatic::tsAutomatic(eWidget *parent): eWidget(parent)
{
	eLabel* l = new eLabel(this);
	l->setName("lNet");
	l_network=new eComboBox(this, 4, l);
	l_network->setName("network");

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");
	
	l_status=new eLabel(this, RS_WRAP);
	l_status->setName("status");

	c_eraseall=new eCheckbox(this);
	c_eraseall->setName("eraseall");
	c_eraseall->hide();

	b_start=new eButton(this);
	b_start->setName("start");
	b_start->hide();
	
	b_abort=new eButton(this);
	b_abort->setName("abort");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsAutomatic"))
		eFatal("skin load of \"tsAutomatic\" failed");

	l_network->setCurrent(new eListBoxEntryText(*l_network, _("automatic"), (void*)0, eTextPara::dirCenter) );

#if 0
	new eListBoxEntryText(l_network, "Astra 19.2°E fake", (void*)"astra192");
	new eListBoxEntryText(l_network, "Astra 19.3°E (bei schiefer Antenne)", (void*)"astra193");
	new eListBoxEntryText(l_network, "Hotbird 11.0°E fuck", (void*)"hb11");
	new eListBoxEntryText(l_network, "Hotbird 13.5°E", (void*)"hbemu135");
	new eListBoxEntryText(l_network, "Hotbird XP", (void*)"hbxp");
	new eListBoxEntryText(l_network, "Sirius -139.2°N", (void*)"sirius-1395");
#endif

	CONNECT(b_start->selected, tsAutomatic::start);
	CONNECT(b_abort->selected, tsAutomatic::abort);
	CONNECT(l_network->selchanged, tsAutomatic::networkSelected);

	CONNECT(eDVB::getInstance()->eventOccured, tsAutomatic::dvbEvent);
	
	if (loadNetworks())
		eFatal("loading networks failed");

	switch (eFrontend::getInstance()->Type())
	{
		case eFrontend::feSatellite:
			l_status->setText(_("To begin searching for a valid satellite press OK, or choose your wished satellite manually and press OK"));
		break;
		case eFrontend::feCable:
			l_status->setText(_("To begin searching for a valid cable provider press OK, or choose your wished cable provider manually and press OK"));
		break;
	}
	
	setFocus(l_network);
}

void tsAutomatic::start()
{
	eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
	if (!sapi)
	{	
		eWarning("no scan active");
		close(1);
	} else
	{
		tpPacket *pkt=(tpPacket*)(l_network->getCurrent() -> getKey());
		for (std::list<eTransponder>::iterator i(pkt->possibleTransponders.begin()); i != pkt->possibleTransponders.end(); ++i)
			sapi->addTransponder(*i);

		// scanflags auswerten
		sapi->setSkipKnownNIT(pkt->scanflags & 8);
		sapi->setUseONIT(pkt->scanflags & 4);
		sapi->setUseBAT(pkt->scanflags & 2);
		sapi->setNetworkSearch(pkt->scanflags & 1);

		// macht nur Probleme...bzw dauert recht lang...
		sapi->setSkipOtherOrbitalPositions(1);

		sapi->setClearList(c_eraseall->isChecked());

		close(0);
	}
}

void tsAutomatic::abort()
{
	close(1);
}

void tsAutomatic::networkSelected(eListBoxEntryText *l)
{
	if (nextNetwork(-1))		// if "automatic" selected,
	{
		automatic=1;
		nextNetwork();				// begin with first
	} else
		automatic=0;
	tuneNext(0);
}

void tsAutomatic::dvbEvent(const eDVBEvent &event)
{
	switch (event.type)
	{
	case eDVBEvent::eventTunedIn:
		eDebug("eventTunedIn");
		if (event.err)
		{
			b_start->hide();
			tuneNext(1);
		} else
		{
			c_eraseall->show();
			b_start->show();
			setFocus(c_eraseall);
			l_status->setText(_("A valid transponder has been found. Verify that the right network is selected"));
		}
		break;
	default:
		break;
	}
}

existNetworks::existNetworks()
:fetype( eFrontend::getInstance()->Type() )
{

}

int existNetworks::parseNetworks()
{
	XMLTreeParser parser("ISO-8859-1");
	
	int done=0;
	const char *filename=0;
	
	switch (fetype)
	{
	case eFrontend::feSatellite:
		filename=CONFIGDIR "/satellites.xml";
		break;
	case eFrontend::feCable:
		filename=CONFIGDIR "/cables.xml";
		break;
	default:
		break;
	}
	
	if (!filename)
		return -1;
		
	FILE *in=fopen(filename, "rt");
	if (!in)
	{
		eWarning("unable to open %s", filename);
		return -1;
	}
	
	do
	{
		char buf[2048];
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser.Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser.ErrorString(parser.GetErrorCode()),
				parser.GetCurrentLineNumber());
			fclose(in);
			return -1;
		}
	} while (!done);
	
	fclose(in);
	
	XMLTreeNode *root=parser.RootNode();
	
	if (!root)
		return -1;
	
	for (XMLTreeNode *node = root->GetChild(); node; node = node->GetNext())
		if (!strcmp(node->GetType(), "cable"))
		{
			tpPacket pkt;
			if (!addNetwork(pkt, node, eFrontend::feCable))
				networks.push_back(pkt);
		} else if (!strcmp(node->GetType(), "sat"))
		{
			tpPacket pkt;
			if (!addNetwork(pkt, node, eFrontend::feSatellite))
				networks.push_back(pkt);
		} else
			eFatal("unknown packet %s", node->GetType());

	return 0;
}

int existNetworks::addNetwork(tpPacket &packet, XMLTreeNode *node, int type)
{
	const char *name=node->GetAttributeValue("name");
	if (!name)
	{
		eFatal("no name");
		return -1;
	}
	packet.name=name;

	const char *flags=node->GetAttributeValue("flags");
	if (flags)
	{
		packet.scanflags=atoi(flags);
		eDebug("name = %s, scanflags = %i", name, packet.scanflags );
	}
	else
	{
		packet.scanflags=1; // default use Network ??
		eDebug("packet has no scanflags... we use default scanflags (1)");
	}
	
	const char *position=node->GetAttributeValue("position");
	if (!position)
		position="0";

	int orbital_position=atoi(position);

	for (node=node->GetChild(); node; node=node->GetNext())
	{
		eTransponder t(*eDVB::getInstance()->settings->getTransponders());
		switch (type)
		{
		case eFrontend::feCable:
		{
			const char *afrequency=node->GetAttributeValue("frequency"),
					*asymbol_rate=node->GetAttributeValue("symbol_rate"),
					*ainversion=node->GetAttributeValue("inversion");
			if (!afrequency)
				continue;
			if (!asymbol_rate)
				asymbol_rate="6900000";
			if (!ainversion)
				ainversion="0";
			int frequency=atoi(afrequency), symbol_rate=atoi(asymbol_rate), inversion=atoi(ainversion);;
			t.setCable(frequency, symbol_rate, inversion);
			break;
		}
		case eFrontend::feSatellite:
		{
			const char *afrequency=node->GetAttributeValue("frequency"),
					*asymbol_rate=node->GetAttributeValue("symbol_rate"),
					*apolarisation=node->GetAttributeValue("polarization"),
					*afec_inner=node->GetAttributeValue("fec_inner"),
					*ainversion=node->GetAttributeValue("inversion");
			if (!afrequency)
				continue;
			if (!asymbol_rate)
				continue;
			if (!apolarisation)
				continue;
			if (!afec_inner)
				continue;
			if (!ainversion)
				ainversion="0";
			int frequency=atoi(afrequency), symbol_rate=atoi(asymbol_rate),
					polarisation=atoi(apolarisation), fec_inner=atoi(afec_inner), inversion=atoi(ainversion);
			t.setSatellite(frequency, symbol_rate, polarisation, fec_inner, orbital_position, inversion);
			break;
		}
		default:
			continue;
		}
		packet.possibleTransponders.push_back(t);
	}
	return 0;
}

int tsAutomatic::loadNetworks()
{
	int err;

	if(	(err = existNetworks::parseNetworks()) )
		return err;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			for (std::list<tpPacket>::const_iterator i(networks.begin()); i != networks.end(); ++i)
				if ( ( i->possibleTransponders.front().satellite.orbital_position == s->getOrbitalPosition() ) || (fetype == eFrontend::feCable) )
					new eListBoxEntryText(*l_network, i->name, (void*)&*i, eTextPara::dirCenter);

	return 0;
}

int tsAutomatic::nextNetwork(int first)
{
	eDebug("next network");

	if (first != -1)
		l_network->moveSelection(first ? eListBox<eListBoxEntryText>::dirFirst : eListBox<eListBoxEntryText>::dirDown);
		
	tpPacket *pkt=(tpPacket*)(l_network->getCurrent() -> getKey());
	
//	eDebug("pkt: %p", pkt);

	if (!pkt)
		return -1;

	current_tp = pkt->possibleTransponders.begin();
	last_tp = pkt->possibleTransponders.end();
	return 0;
}

int tsAutomatic::nextTransponder(int next)
{
	if (next)
		++current_tp;

	if (current_tp == last_tp)
		return 1;

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
		} else
		{
			l_status->setText(_("All known transponders have been tried,"
				" but no lock was possible. Verify antenna-/cable-setup or try another satellite/network."));
			return -1;
		}
		next=0;
	}

	static int i=0;
	i++;
	std::string progress=_("Search in progress ");
	progress+="\\-/|"[i&3];
	l_status->setText(progress);

	return 0;
}

tsText::tsText(eString sheadline, eString sbody, eWidget *parent): eWidget(parent, 1)
{
	addActionMap(&i_cursorActions->map);
	headline=new eLabel(this);
	headline->setText(sheadline);
	headline->setFont(gFont("NimbusSansL-Regular Sans L Regular", 32));
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
			close(0);
		else if (event.action == &i_cursorActions->cancel)
			close(2);
		else
			break;
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

tsScan::tsScan(eWidget *parent): eWidget(parent, 1), timer(eApp)
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

	progress = new eProgress(this);
	progress->setName("scan_progress");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsScan"))
		eFatal("skin load of \"tsScan\" failed");
	
	CONNECT(eDVB::getInstance()->eventOccured, tsScan::dvbEvent);
	CONNECT(eDVB::getInstance()->stateChanged, tsScan::dvbState);
	CONNECT(timer.timeout, tsScan::updateTime);
	CONNECT(eDVB::getInstance()->settings->transponderlist->service_found, tsScan::serviceFound);
	CONNECT(eDVB::getInstance()->settings->transponderlist->transponder_added, tsScan::addedTransponder);
}

int tsScan::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
//		headline->move(ePoint(0, 0));
//		headline->resize(eSize(size.width(), 40));
		return 1;
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->cancel)
			close(2);
		else
			break;
		return 1;
	case eWidgetEvent::execBegin:
	{
		eDVBScanController *sapi=eDVB::getInstance()->getScanAPI();
		if (!sapi)
		{	
			eWarning("no scan active");
			close(1);
		} else
			sapi->start();
		break;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void tsScan::updateTime()
{
		static int scantime=0;
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

	eService *s=eDVB::getInstance()->settings->getTransponders()->searchService(service);
	service_name->setText(s->service_name);
	service_provider->setText(s->service_provider);
	
	if (newService)
	switch(s->service_type)
	{
		case 1:	// digital television service
			newTVServices++;
		break;

		case 2:	// digital radio service
			newRadioServices++;
		break;

		case 3:	// teletext service
		break;

		case 4:	// NVOD reference service
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
	case eDVBScanEvent::eventScanBegin:
			tpLeft = sapi->getknownTransponderSize();
			progress->setPerc(0);
			timer.start(1000);
			tpScanned = newTVServices = newRadioServices = newDataServices = servicesScanned = newTransponders = 0;
		break;
	case eDVBScanEvent::eventScanTPadded:
			tpLeft++;
			perc=(int) ( ( 100.00 / (tpLeft+tpScanned) ) * tpScanned );
			progress->setPerc(perc);
		break;
	case eDVBScanEvent::eventScanNext:
			tpLeft--;
			tpScanned++;
			transponder_scanned->setText(eString().sprintf("%i", tpScanned));
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

TransponderScan::TransponderScan( eWidget *LCDTitle, eWidget *LCDElement)
	:LCDElement(LCDElement), LCDTitle(LCDTitle)
{
	window=new eWindow(0);
	window->setText(_("Transponder Scan"));
	window->cmove(ePoint(100, 100));
	window->cresize(eSize(460, 400));
	
	statusbar=new eStatusBar(window);
	statusbar->loadDeco();
	statusbar->move(ePoint(0, window->getClientSize().height()-30) );
	statusbar->resize( eSize( window->getClientSize().width(), 30 ) );
}

TransponderScan::~TransponderScan()
{
	delete window;
}

int TransponderScan::exec()
{
	eDVB::getInstance()->setMode(eDVB::controllerScan);
	eSize size=eSize(window->getClientSize().width(), window->getClientSize().height()-30);

	eString text;

	enum
	{
		stateMenu,
		stateManual,
		stateAutomatic,
		stateScan,
		stateDone,
		stateEnd
	} state=stateMenu;

	window->show();

	while (state != stateEnd)
	{
		int total=stateEnd;
		
		if (total<2)
			total=2;
		total--;

		switch (state)
		{
		case stateMenu:
		{
			tsSelectType select(window);
			select.setLCD( LCDTitle, LCDElement);
			select.show();
			switch (select.exec())
			{
			case 0:
				state=stateEnd;
				break;
			case 1:
				state=stateAutomatic;
				break;
			case 2:
				state=stateManual;
				break;
			}
			select.hide();
			break;
		}
		case stateManual:
		{
			eTransponder transponder(*eDVB::getInstance()->settings->getTransponders());

			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

			if (sapi && sapi->transponder)
				transponder=*sapi->transponder;
			else
				switch (eFrontend::getInstance()->Type())
				{
				case eFrontend::feCable:
					transponder.setCable(402000, 6900000, 0);	// some cable transponder
					break;
				case eFrontend::feSatellite:
					transponder.setSatellite(12551500, 22000000, eFrontend::polVert, 4, 0, 0);	// some astra transponder
					break;
				default:
					break;
				}

			tsManual manual_scan(window, transponder, LCDTitle, LCDElement);
			manual_scan.show();
			switch (manual_scan.exec())
			{
			case 0:
				state=stateScan;
				break;
			case 1:
				state=stateMenu;
				break;
			}
			manual_scan.hide();
			break;
		}
		case stateAutomatic:
		{
			tsAutomatic automatic_scan(window);
			automatic_scan.setLCD( LCDTitle, LCDElement);
			automatic_scan.show();
			switch (automatic_scan.exec())
			{
			case 0:
				state=stateScan;
				break;
			case 1:
				state=stateMenu;
				break;
			}
			automatic_scan.hide();
			break;
		}
		case stateScan:
		{
			tsScan scan(window);
  		scan.setLCD( LCDTitle, LCDElement);
			scan.move(ePoint(0, 0));
			scan.resize(size);
			
			scan.show();
			statusbar->getLabel().setText(_("Scan is in progress... please wait"));
			scan.exec();
			scan.hide();

			text.sprintf(_("The transponderscan has finished and found %i new Transponders, %i new TV Services, %i new Radio Services and %i new Data Services. %i Transponders within %i Services scanned."), scan.newTransponders, scan.newTVServices, scan.newRadioServices, scan.newDataServices, scan.tpScanned, scan.servicesScanned );
			
			state=stateDone;
			break;
		}
		case stateDone:
		{
			tsText finish(_("Done."), text, window);
  		finish.setLCD( LCDTitle, LCDElement);
  		finish.move(ePoint(0, 0));
			finish.resize(size);
			finish.show();
			statusbar->getLabel().setText(_("Scan is in finished, press ok to close window"));
			finish.exec();
			finish.hide();
			state=stateEnd;
			break;
		}
		default:
			state=stateEnd;
			break;
		}
	}
	
	eDVB::getInstance()->setMode(eDVB::controllerService);

	window->hide();
	return 0;
}
