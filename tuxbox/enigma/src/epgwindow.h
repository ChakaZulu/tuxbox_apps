#ifndef __epgwindow_h
#define __epgwindow_h

#include <lib/gui/listbox.h>
#include <lib/dvb/epgcache.h>
#include <libsig_comp.h>

class eListBoxEntryEPG:public eListBoxEntry
{
	friend class eListBox<eListBoxEntryEPG>;
	friend class eEPGSelector;
	static gFont TimeFont, DescrFont;
	static gPixmap *inTimer, *inTimerRec;
	static int timeXSize, dateXSize;
	int TimeYOffs, DescrYOffs;
	eTextPara *paraDate, *paraTime, *paraDescr;
	EITEvent event;
	tm start_time;
	eString descr;
	eString redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited);
	static int getEntryHeight();
	eServiceReference& service;
public:
	eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox, eServiceReference &ref);
	~eListBoxEntryEPG();
};

class eEPGSelector: public eWindow
{
	eListBox<eListBoxEntryEPG>* events;
	eServiceReferenceDVB current;
private:
	void fillEPGList();
	void entrySelected(eListBoxEntryEPG *entry);
	int eventHandler(const eWidgetEvent &event);
	void invalidateEntry( eListBoxEntryEPG*);
public:
	eEPGSelector(const eServiceReferenceDVB &service);
	inline ~eEPGSelector(){};
	Signal3<bool, eEPGSelector*, eServiceReference*, EITEvent *> addEventToTimerList;
	Signal3<bool, eEPGSelector*, eServiceReference*, EITEvent *> removeEventFromTimerList;
};

#endif
