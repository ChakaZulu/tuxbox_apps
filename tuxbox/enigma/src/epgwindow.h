#ifndef __epgwindow_h
#define __epgwindow_h

#include <core/gui/listbox.h>
#include <core/dvb/epgcache.h>
#include <include/libsig_comp.h>

class eListBoxEntryEPG:public eListBoxEntry
{
	friend class eListBox<eListBoxEntryEPG>;
public:
	static gFont TimeFont;
	static gFont DescrFont;
	static int TimeYOffs, TimeFontHeight;
	ePoint TimeOffs, DescrOffs;
	eTextPara *paraTime, *paraDescr;
	eString descr;
	std::stringstream time;
	EITEvent event;
	eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox);
	~eListBoxEntryEPG();
	void redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited);
};

class eEPGSelector: public eWindow
{
	eListBox<eListBoxEntryEPG>* events;
	eServiceReference current;
private:
	void fillEPGList();
	void entrySelected(eListBoxEntryEPG *entry);
public:
	eEPGSelector(const eServiceReference &service);
	inline ~eEPGSelector(){};
};

#endif
