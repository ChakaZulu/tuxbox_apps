#include "bselect.h"

#include <list>

#include <apps/enigma/enigma.h>
#include <core/gui/eskin.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvb.h>

int eBouquetSelector::fillBouquetList()
{
	int cnt=0;
	bouquets->clearList();
	if (eDVB::getInstance()->settings->getBouquets())
	{
		for (ePtrList<eBouquet>::iterator i(*eDVB::getInstance()->settings->getBouquets()); i != eDVB::getInstance()->settings->getBouquets()->end(); ++i)
		{
      int useable=0;

			for (std::list<eServiceReference>::iterator s = i->list.begin(); (!useable) && s != i->list.end(); s++)
				if ( eZap::getInstance()->getMode() == eZap::TV)
				{
					if (s->service_type == 1 || s->service_type == 4) // Nvod or TV
						useable++;
				}
				else
					if (s->service_type == 2) //Radio
						useable++;

			if (!useable)
				continue;

			new eListBoxEntryBouquet(bouquets, *i);
			cnt++;
		}
		// fake Entry for all services
		if (cnt)
			allServices = new eListBoxEntryBouquet(bouquets, new eBouquet(0, 9999, allServicesName) );

		bouquets->sort();
	}
	bouquets->invalidate();

	return cnt;
}

void eBouquetSelector::entrySelected(eListBoxEntryBouquet *entry)
{
	if (entry)
		result=entry->bouquet;
	else
	{	
		/* emit */ cancel();
		result=0;
	}
	close(1);
}

eBouquetSelector::eBouquetSelector()
	:eWindow(0), allServicesName("* All Services *"), bouquets(new eListBox<eListBoxEntryBouquet>(this))
{
	bouquets->setName("bouquets");
	bouquets->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	if (eSkin::getActive()->build(this, "eBouquetSelector"))
		eWarning("Bouquet selector widget build failed!");
	
	CONNECT(bouquets->selected, eBouquetSelector::entrySelected);
}


eBouquetSelector::~eBouquetSelector()
{
	// remove fake Bouquet (all Services)
	if (allServices)
		delete allServices;
}

eBouquet *eBouquetSelector::choose(int irc)
{
	result=0;
	show();
/*
	if (irc!=-1)
	{
		keyDown(irc);
		keyUp(irc);
	}*/
	if (!exec())
		result=0;
	hide();
	return result;
}

struct moveTo_bouquet_id: public std::unary_function<const eListBoxEntryBouquet&, void>
{
	int bouquet_id;

	moveTo_bouquet_id(int bouquet_id): bouquet_id(bouquet_id)
	{
	}

	bool operator()(const eListBoxEntryBouquet& s)
	{
		if (s.bouquet->bouquet_id == bouquet_id)
		{
	 		( (eListBox<eListBoxEntryBouquet>*) s.listbox)->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};


bool eBouquetSelector::moveTo(int bouquet_id)
{
	bouquets->forEachEntry( moveTo_bouquet_id(bouquet_id) );	
	
	eListBoxEntryBouquet* b = bouquets->getCurrent();

	return b?b->bouquet->bouquet_id == bouquet_id:0;
}	

eBouquet *eBouquetSelector::next()
{
	eListBoxEntryBouquet *s=bouquets->goNext();
	if (s)
		return s->bouquet;
	else
		return 0;
}

eBouquet *eBouquetSelector::prev()
{
	eListBoxEntryBouquet *s=bouquets->goPrev();
	if (s)
		return s->bouquet;
	else
		return 0;
}

eBouquet *eBouquetSelector::current()
{
	eListBoxEntryBouquet* b = bouquets->getCurrent();
	return b?b->bouquet:0;
}
