#ifndef __bselect_h
#define __bselect_h

#include "elbwindow.h"

class eBouquet;
class eLBWindow;

class eBouquetSelector: public eLBWindow
{
	Q_OBJECT
	eBouquet *result;
private slots:
	void fillBouquetList();
	void entrySelected(eListboxEntry *entry);
public:
	eBouquetSelector(eWidget* lcdTitle, eWidget* lcdElement);
	~eBouquetSelector();
	eBouquet *choose(eBouquet *current=0, int irc=-1);
	eBouquet *next();
	eBouquet *prev();
};

#endif
