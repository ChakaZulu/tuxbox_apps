#ifndef __sselect_h
#define __sselect_h

#include "elbwindow.h"
#include "bselect.h"

class eService;
class eLBWindow;
class eBouquet;

class eServiceSelector: public eLBWindow
{
	Q_OBJECT
	eService *result;
	eBouquetSelector* pbs;
protected:
	int eventFilter(const eWidgetEvent &event);
private slots:
	void fillServiceList();
	void entrySelected(eListboxEntry *entry);
public:
	eServiceSelector();
	~eServiceSelector();
	void useBouquet(eBouquet *bouquet);
	eService *choose(eService *current=0, int irc=-1);
	eService *next();
	eService *prev();
};

#endif
