#ifndef __multipage_h_
#define __multipage_h_

#include "qlist.h"
class eWidget;

class eMultipage
{
public:
	eMultipage();
	
	int prev();
	int next();
	void first();
	void addPage(eWidget *page);
	eWidget *current() { return pages.current(); }
	
	int count() { return pages.count(); }
	int at() { return pages.at(); }

	QList<eWidget> pages;
};

#endif
