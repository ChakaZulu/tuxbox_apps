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

struct numberActions
{
	eActionMap map;
	eAction key0, key1, key2, key3, key4, key5, key6, key7, key8, key9;
	numberActions():
		map("numbers", _("number actions")),
		key0(map, "0", _("key 0"), eAction::prioDialog),
		key1(map, "1", _("key 1"), eAction::prioDialog),
		key2(map, "2", _("key 2"), eAction::prioDialog),
		key3(map, "3", _("key 3"), eAction::prioDialog),
		key4(map, "4", _("key 4"), eAction::prioDialog),
		key5(map, "5", _("key 5"), eAction::prioDialog),
		key6(map, "6", _("key 6"), eAction::prioDialog),
		key7(map, "7", _("key 7"), eAction::prioDialog),
		key8(map, "8", _("key 8"), eAction::prioDialog),
		key9(map, "9", _("key 9"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<serviceSelectorActions> i_serviceSelectorActions(5, "service selector actions");
eAutoInitP0<numberActions> i_numberActions(5, "number actions");

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, const eServiceReference &service): 
		eListBoxEntry((eListBox<eListBoxEntry>*)lb), service(service)
{
	bouquet=0;
#if 0
	sort=eString().sprintf("%06d", service->service_number);
#else
	const eService *pservice=eDVB::getInstance()->settings->getTransponders()->searchService(service);
	sort=pservice?pservice->service_name:"";
	sort.upper();
#endif
}

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, eBouquet *bouquet): 
		eListBoxEntry((eListBox<eListBoxEntry>*)lb), bouquet(bouquet)
{
	sort="";
}

eListBoxEntryService::~eListBoxEntryService()
{
}

void eListBoxEntryService::redraw(gPainter *rc, const eRect &rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, bool hilited)
{
	const eService *pservice=eDVB::getInstance()->settings->getTransponders()->searchService(service);
	eString sname;
	if (pservice)
	{
		sname=pservice->service_name;
		EITEvent *e=eEPGCache::getInstance()->lookupCurrentEvent(service);

		eWidget* p = listbox->getParent();			
		if (hilited && p && p->LCDElement)
				p->LCDElement->setText(sort);

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

	rc->setFont(listbox->getFont());

	if ((coNormalB != -1 && !hilited) || (hilited && coActiveB != -1))
	{
		rc->setForegroundColor(hilited?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(hilited?coActiveB:coNormalB);
	} else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}

	rc->setForegroundColor(hilited?coActiveF:coNormalF);

	rc->renderText(rect, sname);
}

struct eServiceSelector_addService: public std::unary_function<eServiceReference&,void>
{
	eListBox<eListBoxEntryService> &list;
	const eServiceReference &result;
	eServiceSelector_addService(eListBox<eListBoxEntryService> &list, const eServiceReference &result): list(list), result(result)
	{
	}
	void operator()(const eServiceReference& c)
	{
		if ((c.service_type!=1) && (c.service_type!=2) && (c.service_type!=4))
			return;
		eListBoxEntryService *l=new eListBoxEntryService(&list, c);
		if (c == result)
			list.setCurrent(l);
	}
};

void eServiceSelector::fillServiceList()
{
	inBouquet = 0;

	setText("full services");

	services->clearList();
	
	if (eDVB::getInstance()->settings->getTransponders())
		eDVB::getInstance()->settings->getTransponders()->forEachServiceReference(eServiceSelector_addService(*services, result?*result:eServiceReference()));

	services->sort();
	services->invalidate();
}

struct moveFirstChar: public std::unary_function<const eListBoxEntryService&, void>
{
	eListBox<eListBoxEntryService> &lb;
	char c;

	moveFirstChar(char c, eListBox<eListBoxEntryService> &lb ): lb(lb), c(c)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		if (s.sort[0] == c)
		{
	 		lb.setCurrent(&s);
			return 1;
		}
		return 0;
	}
};


void eServiceSelector::gotoChar(char c)
{
	switch(c)
	{
		case 2:	// A,B,C
			if (BrowseChar == 'A' || BrowseChar == 'B')
				BrowseChar++;
			else
				BrowseChar = 'A';
		break;
	
		case 3:	// D,E,F
			if (BrowseChar == 'D' || BrowseChar == 'E')
				BrowseChar++;
			else
				BrowseChar = 'D';
		break;

		case 4:	// G,H,I
			if (BrowseChar == 'G' || BrowseChar == 'H')
				BrowseChar++;
			else
				BrowseChar = 'G';
		break;

		case 5:	// J,K,L
			if (BrowseChar == 'J' || BrowseChar == 'M')
				BrowseChar++;
			else
				BrowseChar = 'J';
		break;

		case 6:	// M,N,O
			if (BrowseChar == 'M' || BrowseChar == 'N')
				BrowseChar++;
			else
				BrowseChar = 'M';
		break;

		case 7:	// P,Q,R,S
			if (BrowseChar >= 'P' && BrowseChar <= 'R')
				BrowseChar++;
			else
				BrowseChar = 'P';
		break;

		case 8:	// T,U,V
			if (BrowseChar == 'T' || BrowseChar == 'U')
				BrowseChar++;
			else
				BrowseChar = 'T';
		break;

		case 9:	// W,X,Y,Z
			if (BrowseChar >= 'W' && BrowseChar <= 'Y')
				BrowseChar++;
			else
				BrowseChar = 'W';
		break;
	}
	if (BrowseChar != 0)
	{
		BrowseTimer.start(5000);
		services->forEachEntry(moveFirstChar(BrowseChar, *services));
	}
}

void eServiceSelector::entrySelected(eListBoxEntryService *entry)
{
	if (!entry)
	{
		result=0;
		close(1);
	} else if (entry->service)
	{
		result=&entry->service;
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
			if (event.action == &i_numberActions->key2)
				gotoChar(2);
			else if (event.action == &i_numberActions->key3)
				gotoChar(3);
			else if (event.action == &i_numberActions->key4)
				gotoChar(4);
			else if (event.action == &i_numberActions->key5)
				gotoChar(5);
			else if (event.action == &i_numberActions->key6)
				gotoChar(6);
			else if (event.action == &i_numberActions->key7)
				gotoChar(7);
			else if (event.action == &i_numberActions->key8)
				gotoChar(8);
			else if (event.action == &i_numberActions->key9)
				gotoChar(9);
			else if (event.action == &i_serviceSelectorActions->prevBouquet)
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
				const eventMap* e = eEPGCache::getInstance()->getEventMap(selected);
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
								:eWindow(0), BrowseChar(0), BrowseTimer(eApp)
{
	inBouquet = 0;
	services = new eListBox<eListBoxEntryService>(this);
	services->setName("services");
	services->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	
	pbs = new eBouquetSelector();
	fillServiceList();
	CONNECT(services->selected, eServiceSelector::entrySelected);
	CONNECT(services->selchanged, eServiceSelector::selchanged);
	CONNECT(eDVB::getInstance()->serviceListChanged, eServiceSelector::fillServiceList);
	CONNECT(BrowseTimer.timeout, eServiceSelector::ResetBrowseChar);

	if (eSkin::getActive()->build(this, "eServiceSelector"))
		eWarning("Service selector widget build failed!");

	services->init();
	addActionMap(&i_serviceSelectorActions->map);
	addActionMap(&i_numberActions->map);
}

eServiceSelector::~eServiceSelector()
{
	if (pbs)
		delete pbs;
}

void eServiceSelector::useBouquet(eBouquet *bouquet)
{
	inBouquet = 1;
	services->clearList();
	if (bouquet)
	{
		setText(bouquet->bouquet_name);
		for (std::list<eServiceReference>::iterator i(bouquet->list.begin()); i != bouquet->list.end(); i++)
		{
			if ((i->service_type!=1) && (i->service_type!=2) && (i->service_type!=4))
				continue;
			eListBoxEntryService *l=new eListBoxEntryService(services, *i);
			if (result && (*i == *result) )
				services->setCurrent(l);
		}
		if (bouquet->bouquet_id>=0)
			services->sort();
	}
	services->invalidate();
}

void eServiceSelector::ResetBrowseChar()
{
	BrowseChar=0;
}

const eServiceReference *eServiceSelector::choose(const eServiceReference *current, int irc)
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

const eServiceReference *eServiceSelector::next()
{
	eListBoxEntryService *s=services->goNext();
	if (s)
		return &s->service;
	else
		return 0;
}

const eServiceReference *eServiceSelector::prev()
{
	eListBoxEntryService *s=services->goPrev();
	if (s)
		return &s->service;
	else
		return 0;
}
