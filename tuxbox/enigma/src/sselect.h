#ifndef __sselect_h
#define __sselect_h

#include "elbwindow.h"
#include "bselect.h"

class eService;
class eLBWindow;
class eBouquet;

class eListboxEntryService: public eListboxEntry
{
	eString sort;
public:
	eService *service;
	eBouquet *bouquet;
	eString getText(int col=0) const;
	eListboxEntryService(eService *service, eListbox *listbox);
	eListboxEntryService(eBouquet *bouquet, eListbox *listbox);
	~eListboxEntryService();
};

class eServiceSelector: public eLBWindow
{
	eService *result, *selected;
	eBouquetSelector* pbs;
protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void fillServiceList();
	void entrySelected(eListboxEntry *entry);
	void selchanged(eListboxEntry *entry);
public:
	eServiceSelector();
	~eServiceSelector();
	void useBouquet(eBouquet *bouquet);
	eService *choose(eService *current=0, int irc=-1);
	eService *next();
	eService *prev();
};

#endif
