#ifndef __epgwindow_h
#define __epgwindow_h

#include "elbwindow.h"
#include "epgcache.h"

class eListboxEntryEPG: public eListboxEntry
{
public:
	EITEvent* event;
	QString getText(int col=0) const;
	eListboxEntryEPG(EITEvent* evt, eListbox *listbox);
	~eListboxEntryEPG();
};

class eEPGWindow: public eLBWindow
{
	Q_OBJECT
	eService* current;
protected:
	int eventFilter(const eWidgetEvent &event);
private slots:
	void fillEPGList();
	void entrySelected(eListboxEntry *entry);
public:
	eEPGWindow(eService* service);
	~eEPGWindow();
};

#endif
