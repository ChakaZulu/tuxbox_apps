#ifndef __bselect_h
#define __bselect_h

#include <core/gui/listbox.h>
#include <core/dvb/dvb.h>

class eListBoxEntryBouquet: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryBouquet>;
	friend class eBouquetSelector;
	eBouquet* bouquet;

public:
	eListBoxEntryBouquet(eListBox<eListBoxEntryBouquet>* lb, eBouquet* b)
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, b->bouquet_name), bouquet(b)
	{
	}
};


class eBouquetSelector: public eListBoxWindow<eListBoxEntryBouquet>
{
	eBouquet *result;
private:
	void fillBouquetList();
	void entrySelected(eListBoxEntryBouquet *entry);
public:
	eBouquetSelector();
	~eBouquetSelector();
	eBouquet *choose(eBouquet *current=0, int irc=-1);
	eBouquet *next();
	eBouquet *prev();
};

#endif
