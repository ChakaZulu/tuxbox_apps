#include "sselect.h"
#include "bselect.h"
#include "elbwindow.h"
#include "edvb.h"
#include "dvb.h"
#include "rc.h"
#include "eskin.h"
#include "epgcache.h"
#include <algorithm>
#include "epgwindow.h"

eListboxEntryService::eListboxEntryService(eService &service, eListbox *listbox): eListboxEntry(listbox), service(service)
{
	sort=QString().sprintf("%06d", service.service_number);
}

eListboxEntryService::~eListboxEntryService()
{
}

QString eListboxEntryService::getText(int col=0) const
{
	switch (col)
	{
	case -1:
		return sort;
	case 0:
	{
		QString sname;
		for (unsigned p=0; p<service.service_name.length(); p++)
		{
			QChar ch=service.service_name[p];
			if (ch.unicode()<32)
				continue;
			if (ch.unicode()==0x86)
				continue;
			if (ch.unicode()==0x87)
				continue;
			sname+=ch;
		}
		EITEvent *e=eEPGCache::getInstance()->lookupCurrentEvent(service.original_network_id, service.service_id);
		if (e)
		{
/*			tm* t = localtime(&e->start_time);
			QString _long_description;
			_long_description += QString().sprintf("Start = %02d:%02d", t->tm_hour, t->tm_min);
			time_t endtime = e->start_time+e->duration;
			localtime(&endtime);
			_long_description += QString().sprintf("End %02d:%02d\n", t->tm_hour, t->tm_min);			*/
			for (QListIterator<Descriptor> d(e->descriptor); d.current(); ++d)
			{
				Descriptor *descriptor=d.current();
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
					sname+=" (";
					sname+=ss->event_name;
					sname+=")";
					break;
				}
/*				else if (d.current()->Tag()==DESCR_EXTENDED_EVENT)
				{
					ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)d.current();
					_long_description+=ss->item_description;
				}*/
			}
//			qDebug(_long_description+"\n");
			delete e;
		}
		return sname;
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
		eListboxEntry *l=new eListboxEntryService(c, list);
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
	list->invalidate();
}

void eServiceSelector::entrySelected(eListboxEntry *entry)
{
	if (entry)
		result=&(((eListboxEntryService*)entry)->service);
	else
		result=0;
	close(1);
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
		case eRCInput::RC_HELP:
		{
			eEPGWindow wnd(eDVB::getInstance()->service);
			hide();
			wnd.show();
			wnd.exec();
			wnd.hide();
			show();
			return 1;
		}
		}
		break;
	case eWidgetEvent::keyUp:
		switch (event.parameter)
		{
		}
		break;
	}
	return 0;
}

eServiceSelector::eServiceSelector()
								:eLBWindow("Select Service...", eListbox::tLitebar, 16, eSkin::getActive()->queryValue("fontsize", 20), 600)
{
	pbs = new eBouquetSelector();

	move(QPoint(70, 60));
	
	list->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));

	connect(list, SIGNAL(selected(eListboxEntry*)), SLOT(entrySelected(eListboxEntry*)));
	fillServiceList();
	connect(eDVB::getInstance(), SIGNAL(serviceListChanged()), SLOT(fillServiceList()));
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
		for (QListIterator<eServiceReference> i(bouquet->list); i.current(); ++i)
		{
			eService &c=*i.current()->service;
			if ((c.service_type!=1) && (c.service_type!=2) && (c.service_type!=4))
				return;
			eListboxEntry *l=new eListboxEntryService(c, list);
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
		return &(((eListboxEntryService*)s)->service);
	else
		return 0;
}

eService *eServiceSelector::prev()
{
	eListboxEntry *s=list->goPrev();
	if (s)
		return &(((eListboxEntryService*)s)->service);
	else
		return 0;
}
