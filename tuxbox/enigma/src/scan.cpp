#include <time.h>
#include "scan.h"
#include <core/dvb/frontend.h>
#include <core/dvb/si.h>
#include <core/dvb/dvb.h>
#include "enigma.h"

#include <core/gui/elabel.h>
#include <core/gui/ewindow.h>
#include <core/gdi/font.h>
#include <core/gui/eprogress.h>
#include <core/dvb/edvb.h>
#include <core/driver/rc.h>
#include <core/gui/echeckbox.h>
#include <core/gui/listbox.h>
#include <core/gui/guiactions.h>
#include <core/dvb/dvbwidgets.h>
#include <core/dvb/dvbscan.h>
#include <core/dvb/dvbservice.h>

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

	new eListBoxEntryText(list, "Automatische Suche...", (void*)1);
	new eListBoxEntryText(list, "Manuelle Suche...", (void*)2);
	list->setCurrent(new eListBoxEntryText(list, "Jetzt keine Suche durchführen", (void*)0));

	CONNECT(list->selected, tsSelectType::selected);
}

void tsSelectType::selected(eListBoxEntryText *entry)
{
	if (!entry)
		close(0);
	else
		close((int)entry->getKey());
}

tsManual::tsManual(eWidget *parent, const eTransponder &transponder): eWidget(parent), transponder(transponder)
{
	int ft=0;
	switch (eFrontend::fe()->Type())
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
	
	festatus_widget=new eFEStatusWidget(this, eFrontend::fe());
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
	l_lnb=new eListBox<eListBoxEntryText>(this);
	l_lnb->setName("lnblist");
	l_lnb->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	
	l_network=new eListBox<eListBoxEntryText>(this);
	l_network->setName("network");
	l_network->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);

	eFEStatusWidget *festatus_widget=new eFEStatusWidget(this, eFrontend::fe());
	festatus_widget->setName("festatus");
	
	l_status=new eLabel(this, RS_WRAP);
	l_status->setName("status");

	b_start=new eButton(this);
	b_start->setName("start");
	b_start->hide();
	
	b_abort=new eButton(this);
	b_abort->setName("abort");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "tsAutomatic"))
		eFatal("skin load of \"tsAutomatic\" failed");

	l_lnb->setCurrent(new eListBoxEntryText(l_lnb, "SAT A", (void*)0));
	new eListBoxEntryText(l_lnb, "SAT B", (void*)1);
	new eListBoxEntryText(l_lnb, "SAT A, OPT B", (void*)2);
	new eListBoxEntryText(l_lnb, "SAT B, OPT B", (void*)3);
	
	l_network->setCurrent(new eListBoxEntryText(l_network, _("Automatic"), (void*)0));

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
	CONNECT(l_network->selected, tsAutomatic::networkSelected);

	CONNECT(eDVB::getInstance()->eventOccured, tsAutomatic::dvbEvent);
	
	if (loadNetworks())
		eFatal("loading networks failed");

		// todo: text aendern
	l_status->setText(_("To begin searching for a valid satellite, enter correct LNB and either select satellite or automatic and press OK"));
	
	setFocus(l_lnb);
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
//		sapi->setUseONIT(network->useONIT);
//		sapi->setUseBAT(network->useBAT);
		sapi->setNetworkSearch(1);
		sapi->setClearList(1);
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
			b_start->show();
			setFocus(b_start);
			l_status->setText(_("A valid transponder has been found. Verify that it's the correct LNB and satellite, and press OK to start scanning"));
		}
		break;
	default:
		break;
	}
}

int tsAutomatic::loadNetworks()
{
	int fetype=eFrontend::fe()->Type();

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
		
	for (std::list<tpPacket>::const_iterator i(networks.begin()); i != networks.end(); ++i)
		new eListBoxEntryText(l_network, i->name, (void*)&*i);

	return 0;
}

int tsAutomatic::addNetwork(tpPacket &packet, XMLTreeNode *node, int type)
{
	const char *name=node->GetAttributeValue("name");
	if (!name)
	{
		eFatal("no name");
		return -1;
	}
	
	packet.name=name;
	packet.scanflags=0;

	for (node=node->GetChild(); node; node=node->GetNext())
	{
		eTransponder t;
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
			t.setSatellite(frequency, symbol_rate, polarisation, fec_inner, 0, inversion);
			break;
		}
		default:
			continue;
		}
		packet.possibleTransponders.push_back(t);
	}
	return 0;
}

int tsAutomatic::nextNetwork(int first)
{
	eDebug("next network");

	if (first != -1)
		l_network->moveSelection(first ? eListBox<eListBoxEntryText>::dirFirst : eListBox<eListBoxEntryText>::dirDown);
		
	tpPacket *pkt=(tpPacket*)(l_network->getCurrent() -> getKey());
	
	eDebug("pkt: %p", pkt);

	if (!pkt)
		return -1;

	current_tp = pkt->possibleTransponders.begin();
	last_tp = pkt->possibleTransponders.end();
	return 0;
}

int tsAutomatic::nextTransponder(int next, int lnb)
{
	if (next)
		++current_tp;

	if (current_tp == last_tp)
		return 1;

	if (current_tp->satellite.valid)
		current_tp->satellite.lnb=lnb;
	return current_tp->tune();
}

int tsAutomatic::tuneNext(int next)
{
	while (nextTransponder(next, (int)l_lnb->getCurrent()->getKey()))
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

tsScan::tsScan(eWidget *parent): eWidget(parent, 1)
{
	addActionMap(&i_cursorActions->map);
	headline=new eLabel(this);
	headline->setText("Scanning...");
	
	CONNECT(eDVB::getInstance()->eventOccured, tsScan::dvbEvent);
	CONNECT(eDVB::getInstance()->stateChanged, tsScan::dvbState);
}

int tsScan::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		headline->move(ePoint(0, 0));
		headline->resize(eSize(size.width(), 40));
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

void tsScan::dvbEvent(const eDVBEvent &event)
{
	switch (event.type)
	{
	case eDVBScanEvent::eventScanNext:
		// update();
		break;
	case eDVBScanEvent::eventScanCompleted:
		close(0);
		break;
	default:
		break;
	}
}

void tsScan::dvbState(const eDVBState &state)
{
}

TransponderScan::TransponderScan()
{

	window=new eWindow(0);
	window->setText("Transponder Scan");
	window->cmove(ePoint(100, 100));
	window->cresize(eSize(460, 400));
	
	eSize size=eSize(window->getClientSize().width(), window->getClientSize().height()-30);

	progress=new eProgress(window);
	progress->move(ePoint(60, size.height()+5));
	progress->resize(eSize(window->getClientSize().width()-70, 10));

	progress_text=new eLabel(window);
	progress_text->move(ePoint(0, size.height()));
	progress_text->resize(eSize(50, 30));
}

TransponderScan::~TransponderScan()
{
	delete window;
}

int TransponderScan::exec()
{
	eDVB::getInstance()->setMode(eDVB::controllerScan);
	eSize size=eSize(window->getClientSize().width(), window->getClientSize().height()-30);

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
		
		progress_text->setText(eString().sprintf("%d/%d", state+1, total));
		if (total<2)
			total=2;
		total--;
		progress->setPerc(state*100/total);

		switch (state)
		{
		case stateMenu:
		{
			tsSelectType select(window);
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
			eTransponder transponder;

			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

			if (sapi && sapi->transponder)
				transponder=*sapi->transponder;
			else
				switch (eFrontend::fe()->Type())
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

			tsManual manual_scan(window, transponder);
			
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
			scan.move(ePoint(0, 0));
			scan.resize(size);
			
			scan.show();
			scan.exec();
			scan.hide();
			
			state=stateDone;
			break;
		}
		case stateDone:
		{
			tsText finish(_("Done."), _("The transponderscan has finished and found n new Transponders with n new services."), window);
			finish.move(ePoint(0, 0));
			finish.resize(size);
			finish.show();
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
