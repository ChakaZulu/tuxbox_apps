#ifndef __sselect_h
#define __sselect_h

#include <core/gui/ewindow.h>
#include <apps/enigma/bselect.h>
#include <core/gui/listbox.h>

class eService;
class eBouquet;
class eLabel;

class eListBoxEntryService: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryService>;
	eString sort;
	eString short_name;
public:
	eService *service;
	eBouquet *bouquet;
	eString getText(int col=0) const;
	eListBoxEntryService(eListBox<eListBoxEntryService> *lb, eService *service);
	eListBoxEntryService(eListBox<eListBoxEntryService> *lb, eBouquet *bouquet);
	~eListBoxEntryService();
	
	bool operator<(const eListBoxEntryService &r) const
	{
		return sort < r.sort;
	}
protected:
	void redraw(gPainter *rc, const eRect &rect, const gColor &coActive, const gColor &coNormal, bool highlighted);
};

class eServiceSelector: public eWindow
{
	eService *result, *selected;
	
	eListBox<eListBoxEntryService> *services;

	eBouquetSelector* pbs;
protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void fillServiceList();
	void entrySelected(eListBoxEntryService *entry);
	void selchanged(eListBoxEntryService *entry);
public:
	enum
	{
		dirNo,
		dirUp,
		dirDown
	};
	eServiceSelector();
	~eServiceSelector();
	void useBouquet(eBouquet *bouquet);
	eService *choose(eService *current=0, int irc=-1);
	eService *next();
	eService *prev();
};

#endif
