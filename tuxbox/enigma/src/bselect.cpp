#include "bselect.h"
#include "elbwindow.h"
#include "elistbox.h"
#include "edvb.h"
#include "dvb.h"
#include "eskin.h"
#include <list>

void eBouquetSelector::fillBouquetList()
{
	list->clearList();
	if (eDVB::getInstance()->getBouquets())
	{
		for (ePtrList<eBouquet>::iterator i(*eDVB::getInstance()->getBouquets()); i != eDVB::getInstance()->getBouquets()->end(); ++i)
		{
			int usable=0;
			for (std::list<eServiceReference>::iterator s = i->list.begin(); (!usable) && s != i->list.end(); s++)
			{
				if (!s->service)
					continue;
				int st=s->service->service_type;
				if ((st==1) || (st==2) || (st==4))
					usable=1;
			}
			if (!usable)
				continue;
			eBouquet *c=*i;
			eListboxEntry *l=new eListboxEntryText(list, c->bouquet_name.c_str(), 0, c);
			if (c==result)
				list->setCurrent(l);
		}
		list->sort();
	}
	list->invalidate();
}

void eBouquetSelector::entrySelected(eListboxEntry *entry)
{
	if (entry)
		result=(eBouquet*)entry->data;
	else
		result=0;
	close(1);
}

eBouquetSelector::eBouquetSelector()
								:eLBWindow("Select Bouquet...", eListbox::tLitebar, 17, eSkin::getActive()->queryValue("fontsize", 20), 400)
{
	move(ePoint(80, 60));
/*	connect(list, SIGNAL(selected(eListboxEntry*)), SLOT(entrySelected(eListboxEntry*)));
	connect(eDVB::getInstance(), SIGNAL(bouquetListChanged()), SLOT(fillBouquetList()));*/
	CONNECT(list->selected, eBouquetSelector::entrySelected);
	CONNECT(eDVB::getInstance()->bouquetListChanged, eBouquetSelector::fillBouquetList);
	fillBouquetList();
}


eBouquetSelector::~eBouquetSelector()
{
}

eBouquet *eBouquetSelector::choose(eBouquet *current, int irc)
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

eBouquet *eBouquetSelector::next()
{
	eListboxEntry *s=list->goNext();
	if (s)
		return (eBouquet*)s->data;
	else
		return 0;
}

eBouquet *eBouquetSelector::prev()
{
	eListboxEntry *s=list->goPrev();
	if (s)
		return (eBouquet*)s->data;
	else
		return 0;
}
