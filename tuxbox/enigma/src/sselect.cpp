#include <algorithm>
#include <list>

#include "sselect.h"
#include "bselect.h"
#include "epgwindow.h"

#include <core/gui/elbwindow.h>
#include <core/gui/eskin.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvb.h>
#include <core/dvb/epgcache.h>
#include <core/driver/rc.h>

eListboxEntryService::eListboxEntryService(eService *service, eListbox *listbox): eListboxEntry(listbox), service(service)
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

eListboxEntryService::eListboxEntryService(eBouquet *service, eListbox *listbox): eListboxEntry(listbox), bouquet(bouquet)
{
	service=0;
	sort="";
}

eListboxEntryService::~eListboxEntryService()
{
}

eString eListboxEntryService::getText(int col=0) const
{
	switch (col)
	{
	case -1:
		return sort;
	case 0:
	{
		if (service)
		{
			eString sname;
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
			return sname;
		}
		if (bouquet)
		{
			return bouquet->bouquet_name;
		}
		return 0;
	}
	default:
		return 0;
	}
}

struct eServiceSelector_addService: public std::unary_function<eService&,void>
{
	eListbox *list;
	eService *result;
	eServiceSelector_addService(eListbox *list, eService *result): list(list), result(result)
	{
	}
	void operator()(eService& c)
	{
		if ((c.service_type!=1) && (c.service_type!=2) && (c.service_type!=4))
			return;
		eListboxEntry *l=new eListboxEntryService(&c, list);
		if (&c==result)
			list->setCurrent(l);
	}
};

void eServiceSelector::fillServiceList()
{
	setText("full services");
	list->clearList();
	if (eDVB::getInstance()->getTransponders())
		eDVB::getInstance()->getTransponders()->forEachChannel(eServiceSelector_addService(list, result));
	list->sort();
	list->invalidate();
}

void eServiceSelector::entrySelected(eListboxEntry *e)
{
	eListboxEntryService *entry=(eListboxEntryService*)e;
	if (!entry)
		result=0;
	else if (entry->service)
	{
		result=((eListboxEntryService*)entry)->service;
		close(1);
	} else if (entry->bouquet)
		useBouquet(entry->bouquet);
}

void eServiceSelector::selchanged(eListboxEntry *entry)
{
	selected = (((eListboxEntryService*)entry)->service);
}

int eServiceSelector::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::keyDown:
			switch (event.parameter)
			{
			case eRCInput::RC_PLUS:
			{
				eBouquet *b;
				b=pbs->next();
				if (b)
					useBouquet(b);
				return 1;
			}
			case eRCInput::RC_MINUS:
			{
				eBouquet *b;
				b=pbs->prev();
				if (b)
					useBouquet(b);
				return 1;
			}
			case eRCInput::RC_DBOX:
			{
				eBouquet *b;
				hide();
				pbs->setLCD(LCDTitle, LCDElement);
				b=pbs->choose();
				if (b)
					useBouquet(b);
				show();
				return 1;
			}
			case eRCInput::RC_HOME:
			{
				fillServiceList();
				return 1;
			}
		}
		break;
		case eWidgetEvent::keyUp:
			switch (event.parameter)
			{
			case eRCInput::RC_RED:
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
				return 1;
			}
			case eRCInput::RC_HELP:
				close(0);
				break;
			}
		break;
	}
	return 0;
}

eServiceSelector::eServiceSelector()
								:eLBWindow("Select Service...", eListbox::tLitebar, 16, eSkin::getActive()->queryValue("fontsize", 20), 600)
{
	pbs = new eBouquetSelector();
	move(ePoint(70, 60));
	list->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));
	fillServiceList();
	CONNECT(list->selected, eServiceSelector::entrySelected);
	CONNECT(list->selchanged, eServiceSelector::selchanged);
	CONNECT(eDVB::getInstance()->serviceListChanged, eServiceSelector::fillServiceList);
}

eServiceSelector::~eServiceSelector()
{
	if (pbs)
		delete pbs;
}

void eServiceSelector::useBouquet(eBouquet *bouquet)
{
	list->clearList();
	if (bouquet)
	{
		setText(bouquet->bouquet_name);
		for (std::list<eServiceReference>::iterator i(bouquet->list.begin()); i != bouquet->list.end(); i++)
		{
			if (!i->service)
				continue;
			eService &c = *i->service;
			if ((c.service_type!=1) && (c.service_type!=2) && (c.service_type!=4))
				continue;
			eListboxEntry *l=new eListboxEntryService(&c, list);
			if (&c==result)
				list->setCurrent(l);
		}
		if (bouquet->bouquet_id>=0)
			list->sort();
	}
	list->invalidate();
}

eService *eServiceSelector::choose(eService *current, int irc)
{
	result=current;
	show();
	if (irc!=-1)
	{
		event(eWidgetEvent(eWidgetEvent::keyDown, irc));
		event(eWidgetEvent(eWidgetEvent::keyUp, irc));
	}
	if (!exec())
		result=0;
	hide();
	return result;
}

eService *eServiceSelector::next()
{
	eListboxEntry *s=list->goNext();
	if (s)
		return (((eListboxEntryService*)s)->service);
	else
		return 0;
}

eService *eServiceSelector::prev()
{
	eListboxEntry *s=list->goPrev();
	if (s)
		return (((eListboxEntryService*)s)->service);
	else
		return 0;
}
