#ifndef __epgwindow_h
#define __epgwindow_h

#include "elbwindow.h"
#include "epgcache.h"
#include <libsig_comp.h>

class eListboxEntryEPG: public eListboxEntry
{
public:
	EITEvent* event;
	eString getText(int col=0) const;
	inline eListboxEntryEPG(EITEvent* evt, eListbox *listbox): eListboxEntry(listbox), event(evt)	{	}
	inline ~eListboxEntryEPG(){ delete event; }
	inline int operator<(const eListboxEntry& q) const;
	inline int operator==(const eListboxEntry& q) const;
};

int eListboxEntryEPG::operator<(const eListboxEntry & q) const
{
	// much faster as string compare
	return 0;//( event->start_time < ((eListboxEntryEPG*) &q)->event->start_time );
}

int eListboxEntryEPG::operator==(const eListboxEntry & q) const
{
	return (event->start_time == ((eListboxEntryEPG*) &q)->event->start_time );
}

class eEPGWindow: public eLBWindow
{
	eService* current;
private:
	void fillEPGList();
	void entrySelected(eListboxEntry *entry);
	void closeWnd();
public:
	eEPGWindow(eService* service);
	inline ~eEPGWindow(){};
};

#endif
