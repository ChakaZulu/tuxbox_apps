#include <algorithm>
#include <list>

#include "sselect.h"
#include "bselect.h"
#include "epgwindow.h"

#include <core/base/i18n.h>
#include <core/gui/actions.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvb.h>
#include <core/dvb/epgcache.h>
#include <core/driver/rc.h>
#include <core/system/init.h>

struct serviceSelectorActions
{
  eActionMap map;
	eAction nextBouquet, prevBouquet, showBouquetSelector, showEPGSelector, showAllServices;
	serviceSelectorActions():
		map("serviceSelector", _("service selector")),
		nextBouquet(map, "nextBouquet", _("switch to next bouquet"), eAction::prioDialogHi),
		prevBouquet(map, "prevBouquet", _("switch to previous bouquet"), eAction::prioDialogHi),
		showBouquetSelector(map, "showBouquetSelector", _("shows the bouquet selector"), eAction::prioDialog),
		showEPGSelector(map, "showEPGSelector", _("shows the EPG selector for the highlighted channel"), eAction::prioDialog),
		showAllServices(map, "showAllServices", _("switch to all services"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<serviceSelectorActions> i_serviceSelectorActions(5, "service selector actions");

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, eService *service): 
		eListBoxEntry((eListBox<eListBoxEntry>*)lb), service(service)
{
	bouquet=0;
#if 0
	sort=eString().sprintf("%06d", service->service_number);
#else
	sort="";
	for (unsigned p=0; p<service->service_name.length(); p++)
	{
		char ch=service->service_name[p];
		if (ch<32)
			continue;
		if (ch==0x86)

			continue;
		if (ch==0x87)
			continue;
		sort+=ch;
	}
#endif
}

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, eBouquet *bouquet): 
		eListBoxEntry((eListBox<eListBoxEntry>*)lb), bouquet(bouquet)
{
	service=0;
	sort="";
}

eListBoxEntryService::~eListBoxEntryService()
{
}

void eListBoxEntryService::redraw(gPainter *rc, const eRect &rect, const gColor &coActive, const gColor &coNormal, bool highlighted)
{
	eString sname;
	if (service)
	{
		for (unsigned p=0; p<service->service_name.length(); p++)
		{
			char ch=service->service_name[p];
			if (ch<32)
				continue;
			if (ch==0x86)
				continue;
			if (ch==0x87)
				continue;
			sname+=ch;
		}
		EITEvent *e=eEPGCache::getInstance()->lookupCurrentEvent(service->original_network_id, service->service_id);
		if (e)
		{
			for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
					sname+=" (";
					sname+=ss->event_name;
					sname+=")";
					break;
				}
			}
			delete e;
		}
	} else if (bouquet)
		sname=bouquet->bouquet_name;
	else
		return;
	gColor col=highlighted?coActive:coNormal;
	rc->setForegroundColor(col);
	rc->setFont(listbox->getEntryFnt());
	if (col != -1)
		rc->fill(rect);
	rc->renderText(rect, sname);
}

struct eServiceSelector_addService: public std::unary_function<eService&,void>
{
	eListBox<eListBoxEntryService> &list;
	eService *result;
	eServiceSelector_addService(eListBox<eListBoxEntryService> &list, eService *result): list(list), result(result)
	{
	}
	void operator()(eService& c)
	{
		if ((c.service_type!=1) && (c.service_type!=2) && (c.service_type!=4))
			return;
		eListBoxEntryService *l=new eListBoxEntryService(&list, &c);
		if (&c==result)
			list.setCurrent(l);
	}
};

void eServiceSelector::fillServiceList()
{
	setText("full services");

	services->clearList();
	
	if (eDVB::getInstance()->settings->getTransponders())
		eDVB::getInstance()->settings->getTransponders()->forEachChannel(eServiceSelector_addService(*services, result));

	services->sort();
	services->invalidate();
}

void eServiceSelector::entrySelected(eListBoxEntryService *entry)
{
	if (!entry)
	{
		result=0;
		close(1);
	} else if (entry->service)
	{
		result=entry->service;
		close(0);
	} else if (entry->bouquet)
		useBouquet(entry->bouquet);
}

void eServiceSelector::selchanged(eListBoxEntryService *entry)
{
	selected = (((eListBoxEntryService*)entry)->service);
}

int eServiceSelector::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_serviceSelectorActions->prevBouquet)
			{
				eBouquet *b;
				b=pbs->prev();
				if (b)
					useBouquet(b);
			}
			else if (event.action == &i_serviceSelectorActions->nextBouquet)
			{
					eBouquet *b;
					b=pbs->next();
					if (b)
						useBouquet(b);
			}
			else if (event.action == &i_serviceSelectorActions->showBouquetSelector)
			{
					eBouquet *b;
					hide();
					pbs->setLCD(LCDTitle, LCDElement);
					b=pbs->choose();
					if (b)
						useBouquet(b);
					show();
			}
			else if (event.action == &i_serviceSelectorActions->showEPGSelector)
			{
					const eventMap* e = eEPGCache::getInstance()->getEventMap(selected->original_network_id, selected->service_id);
					if (e && !e->empty())
					{
						eEPGWindow wnd(selected);
						if (LCDElement && LCDTitle)
							wnd.setLCD(LCDTitle, LCDElement);
						hide();
						wnd.show();
						wnd.exec();
						wnd.hide();
						show();
					}
			}
			else if (event.action == &i_serviceSelectorActions->showAllServices)
				fillServiceList();
			else
				break;
			return 1;

		default:
			break;
	}
	return eWindow::eventHandler(event);
}

eServiceSelector::eServiceSelector()
								:eWindow(0)
{
	services = new eListBox<eListBoxEntryService>(this);
	services->setName("services");
	services->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));
	
	l_bouquet = new eLabel(this);
	l_bouquet->setName("bouquet");
	
	l_bouquet_prev = new eLabel(this);
	l_bouquet_prev->setName("bouquet_prev");
	
	l_bouquet_next = new eLabel(this);
	l_bouquet_next->setName("bouquet_next");
	
	pbs = new eBouquetSelector();
	fillServiceList();
	CONNECT(services->selected, eServiceSelector::entrySelected);
	CONNECT(services->selchanged, eServiceSelector::selchanged);
	CONNECT(eDVB::getInstance()->serviceListChanged, eServiceSelector::fillServiceList);

	if (eSkin::getActive()->build(this, "eServiceSelector"))
		eWarning("Service selector widget build failed!");

	services->init();
	addActionMap(&i_serviceSelectorActions->map);
}

eServiceSelector::~eServiceSelector()
{
	if (pbs)
		delete pbs;
}

void eServiceSelector::useBouquet(eBouquet *bouquet)
{
	services->clearList();
	if (bouquet)
	{
		l_bouquet->setText(bouquet->bouquet_name);
		for (std::list<eServiceReference>::iterator i(bouquet->list.begin()); i != bouquet->list.end(); i++)
		{
			if (!i->service)
				continue;
			eService &c = *i->service;
			if ((c.service_type!=1) && (c.service_type!=2) && (c.service_type!=4))
				continue;
			eListBoxEntryService *l=new eListBoxEntryService(services, &c);
			if (&c==result)
				services->setCurrent(l);
		}
		if (bouquet->bouquet_id>=0)
			services->sort();
	}
	services->invalidate();
}

eService *eServiceSelector::choose(eService *current, int irc)
{
	result=current;
	switch (irc)
	{
	case dirUp:
		services->moveSelection(eListBox<eListBoxEntryService>::dirUp);
		break;
	case dirDown:
		services->moveSelection(eListBox<eListBoxEntryService>::dirDown);
		break;
	default:
		break;
	}
	show();
	if (exec())
		result=0;
	hide();
	return result;
}

eService *eServiceSelector::next()
{
	eListBoxEntryService *s=services->goNext();
	if (s)
		return s->service;
	else
		return 0;
}

eService *eServiceSelector::prev()
{
	eListBoxEntryService *s=services->goPrev();
	if (s)
		return s->service;
	else
		return 0;
}
