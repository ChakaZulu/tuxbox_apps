#ifndef __epgwindow_h
#define __epgwindow_h

#include "eListBox.h"
#include "epgcache.h"
#include <libsig_comp.h>

class eListBoxEntryEPG: public eListBoxEntryTextStream
{
	friend class eListBox<eListBoxEntryEPG>;
public:
	EITEvent event;
	eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox);
};

class eEPGWindow: public eListBoxWindow<eListBoxEntryEPG>
{
	eService* current;
private:
	void fillEPGList();
	void entrySelected(eListBoxEntryEPG *entry);
public:
	eEPGWindow(eService* service);
	inline ~eEPGWindow(){};
};

#endif
