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

tsManual::tsManual(eWidget *parent, const eTransponder &transponder, tpScanParameter &param): eWidget(parent), param(param), transponder(transponder)
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

	select_type=new tsSelectType(window);
	select_type->hide();
	mp.addPage(select_type);

	manual_scan=new tsManual(window, transponder, scanparam);
	manual_scan->hide();
	mp.addPage(manual_scan);
	
	eWidget *scan;
	scan=new tsScan(window);
	scan->hide();
	scan->move(ePoint(0, 0));
	scan->resize(size);
	mp.addPage(scan);

	tsText *finish=new tsText(_("Done."), _("The transponderscan has finished and found n new Transponders with n new services."), window);
	finish->hide();
	finish->move(ePoint(0, 0));
	finish->resize(size);
	mp.addPage(finish);
}

TransponderScan::~TransponderScan()
{
	delete window;
}

int TransponderScan::exec()
{
	int ret;
	window->show();
	mp.first();
	
	eDVB::getInstance()->setMode(eDVB::controllerScan);

	ret=0;
	while (!ret)
	{
		int n=mp.at();
		int total=mp.count();
		progress_text->setText(eString().sprintf("%d/%d", n+1, total));
		if (total<2)
			total=2;
		total--;
		progress->setPerc(n*100/total);
		int res=mp.getCurrent()->exec();
			
		if (mp.getCurrent()==select_type)
		{
			switch (res)
			{
			case 0:
				ret=-1;
				break;
			case 1:
				mp.set(automatic_scan);
				break;
			case 2:
				mp.set(manual_scan);
				break;
			}
		} else
		{
			switch (res)
			{
			case 0:
				if (mp.next())
					ret=1;
				break;
			case 1:
				mp.prev();
				break;
			case 2:
				ret=-1;
				break;
			}
		}
	}

	eDVB::getInstance()->setMode(eDVB::controllerService);

	mp.getCurrent()->hide();
	window->hide();
	return ret;
}
