#ifndef __bselect_h
#define __bselect_h

#include <core/gui/elbwindow.h>

class eBouquet;
class eLBWindow;

class eBouquetSelector: public eLBWindow
{
	eBouquet *result;
private:
	void fillBouquetList();
	void entrySelected(eListboxEntry *entry);
public:
	eBouquetSelector();
	~eBouquetSelector();
	eBouquet *choose(eBouquet *current=0, int irc=-1);
	eBouquet *next();
	eBouquet *prev();
};

#endif
