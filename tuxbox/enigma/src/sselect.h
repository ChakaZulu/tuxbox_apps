#ifndef __sselect_h
#define __sselect_h

#include "elbwindow.h"
#include "bselect.h"

class eService;
class eLBWindow;
class eBouquet;

class eListboxEntryService: public eListboxEntry
{
	QString sort;
public:
	eService &service;
	QString getText(int col=0) const;
	eListboxEntryService(eService &service, eListbox *listbox);
	~eListboxEntryService();
};

class eServiceSelector: public eLBWindow
{
//	Q_OBJECT
	eService *result, *selected;
	eBouquetSelector* pbs;
protected:
	int eventFilter(const eWidgetEvent &event);
private:// slots:
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
