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
	friend struct moveFirstChar;
	friend struct selectService;
	eString sort;
	eString short_name;
	static gFont serviceFont;
	static gFont descrFont;
	static gFont numberFont;
public:
	eServiceReference service;
	eListBoxEntryService(eListBox<eListBoxEntryService> *lb, const eServiceReference &service);
	~eListBoxEntryService();
	
	bool operator<(const eListBoxEntryService &r) const
	{
		return sort < r.sort;
	}

protected:
	void redraw(gPainter *rc, const eRect &rect, gColor, gColor, gColor, gColor, bool highlighted);
};

class eServiceSelector: public eWindow
{
	int lastTvBouquet, lastRadioBouquet, inFullServiceList;
	bool selectioChanged;

	eServiceReference selected;
	eServiceReference *result;
	eListBox<eListBoxEntryService> *services;

	eBouquetSelector* pbs;
protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void fillServiceList();
	void entrySelected(eListBoxEntryService *entry);
	void selchanged(eListBoxEntryService *entry);
	char BrowseChar;
	eTimer BrowseTimer;
	void ResetBrowseChar();
	void gotoChar(char c);
public:
	enum
	{
		dirNo,
		dirUp,
		dirDown
	};
	void actualize(int currentService, int dum=0);
	void selectCurrentService();
	void resetBouquet();
	eServiceSelector(int currentService);
	~eServiceSelector();
	void useBouquet(eBouquet *bouquet);
	const eServiceReference *choose(int irc=-1);
	const eServiceReference *next();
	const eServiceReference *prev();
};

#endif
