#include "sselect.h"
#include "bselect.h"
#include "elbwindow.h"
#include "edvb.h"
#include "dvb.h"
#include "rc.h"
#include "ezap.h"
#include "eskin.h"

void eServiceSelector::fillServiceList()
{
	setText("full services");
	list->clearList();
	if (eDVB::getInstance()->getServices())
	{
		for (QListIterator<eService> i(*eDVB::getInstance()->getServices()); i.current(); ++i)
		{
			eService *c=i.current();
			if ((c->service_type!=1) && (c->service_type!=2) && (c->service_type!=4))
				continue;
			QString sname;
			for (unsigned p=0; p<c->service_name.length(); p++)
			{
				QChar ch=c->service_name[p];
				if (ch.unicode()<32)
					continue;
				if (ch.unicode()==0x86)
					continue;
				if (ch.unicode()==0x87)
					continue;
				sname+=ch;
			}
			eListboxEntry *l=new eListboxEntryText(list, sname, QString().sprintf("%06d", c->service_number), c);
			if (c==result)
				list->setCurrent(l);
		}
		list->sort();
	}
	list->redraw();
}

void eServiceSelector::entrySelected(eListboxEntry *entry)
{
	if (entry)
		result=(eService*)entry->data;
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
			b=bs.next();
			if (b)
				useBouquet(b);
			return 1;
		}
		case eRCInput::RC_MINUS:
		{
			eBouquet *b;
			b=bs.prev();
			if (b)
				useBouquet(b);
			return 1;
		}
		}
		break;
	case eWidgetEvent::keyUp:
		switch (event.parameter)
		{
		case eRCInput::RC_DBOX:
		{
			eBouquet *b;
			hide();
			b=bs.choose();
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
	}
	return 0;
}

eServiceSelector::eServiceSelector(): eLBWindow("Select Service...", eListbox::tLitebar, 17, eZap::FontSize, 400)
{
	move(QPoint(70, 50));
	
	list->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));

	connect(list, SIGNAL(selected(eListboxEntry*)), SLOT(entrySelected(eListboxEntry*)));
	fillServiceList();
	connect(eDVB::getInstance(), SIGNAL(serviceListChanged()), SLOT(fillServiceList()));
}

eServiceSelector::~eServiceSelector()
{
}

void eServiceSelector::useBouquet(eBouquet *bouquet)
{
	list->clearList();
	if (bouquet)
	{
		setText(bouquet->bouquet_name);
		for (QListIterator<eServiceReference> i(bouquet->list); i.current(); ++i)
		{
			eService *c=i.current()->service;
			if (!c)
				 continue;
			if ((c->service_type!=1) && (c->service_type!=2) && (c->service_type!=4))
				continue;
			QString sname;
			for (unsigned p=0; p<c->service_name.length(); p++)
			{
				QChar ch=c->service_name[p];
				if (ch.unicode()<32)
					continue;
				if (ch.unicode()==0x86)
					continue;
				if (ch.unicode()==0x87)
					continue;
				sname+=ch;
			}
			eListboxEntry *l=new eListboxEntryText(list, sname, QString().sprintf("%06d", c->service_number), c);
			if (c==result)
				list->setCurrent(l);
		}
		if (bouquet->bouquet_id>=0)
			list->sort();
	}
	list->redraw();
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
		return (eService*)s->data;
	else
		return 0;
}

eService *eServiceSelector::prev()
{
	eListboxEntry *s=list->goPrev();
	if (s)
		return (eService*)s->data;
	else
		return 0;
}
