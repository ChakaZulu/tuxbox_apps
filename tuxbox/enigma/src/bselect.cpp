#include "bselect.h"

#include <list>

#include <core/gui/eskin.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvb.h>

void eBouquetSelector::fillBouquetList()
{
	list.clearList();
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

			eListBoxEntryBouquet *l=new eListBoxEntryBouquet(&list, *i);

			if (*i==result)
				list.setCurrent(l);
		}
		list.sort();
	}
	list.invalidate();
}

void eBouquetSelector::entrySelected(eListBoxEntryBouquet *entry)
{
	if (entry)
		result=entry->bouquet;
	else
		result=0;
	close(1);
}

eBouquetSelector::eBouquetSelector()
								:eListBoxWindow<eListBoxEntryBouquet>("Select Bouquet...", 17, eSkin::getActive()->queryValue("fontsize", 20), 400)
{
	move(ePoint(80, 60));
	CONNECT(list.selected, eBouquetSelector::entrySelected);
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
		keyDown(irc);
		keyUp(irc);
	}
	if (!exec())
		result=0;
	hide();
	return result;
}

eBouquet *eBouquetSelector::next()
{
	eListBoxEntryBouquet *s=list.goNext();
	if (s)
		return s->bouquet;
	else
		return 0;
}

eBouquet *eBouquetSelector::prev()
{
	eListBoxEntryBouquet *s=list.goPrev();
	if (s)
		return s->bouquet;
	else
		return 0;
}
