#include <errno.h>
#include "multipage.h"
#include "ewidget.h"

eMultipage::eMultipage()
{
}

int eMultipage::prev()
{
	if (pages.current()==pages.getFirst())
		return -ENOENT;
	if (!pages.current())
		return -ENOENT;
	pages.current()->hide();
	pages.prev();
	pages.current()->show();
	return 0;
}

int eMultipage::next()
{
	if (pages.current()==pages.getLast())
		return -ENOENT;
	if (!pages.current())
		return -ENOENT;
	pages.current()->hide();
	pages.next();
	pages.current()->show();
	return 0;
}

void eMultipage::first()
{
	if (pages.current()==pages.getFirst())
		return;
	if (pages.current())
		pages.current()->hide();
	pages.first();
	pages.current()->show();
}

void eMultipage::addPage(eWidget *page)
{
	pages.append(page);
}
