#include "elistbox.h"
#include "fb.h"
#include "elabel.h"
#include "init.h"
#include <core/gui/guiactions.h>

eListbox::eListbox(eWidget *parent, int ih)
	 :eWidget(parent, 1)
{
	col_active=eSkin::getActive()->queryScheme("focusedColor");
	top=0;
	bottom=0;
	current=0;
	font_size=ih;
	item_height=ih+2;
	have_focus=0;
	entryFnt=gFont("NimbusSansL-Regular Sans L Regular", font_size);
	childs.setAutoDelete(true);
	
	addActionMap(&i_cursorActions->map);
	addActionMap(&i_listActions->map);
}

void eListbox::redrawWidget(gPainter *target, const eRect &where)
{
	if (!top)
		return;
	int i=0;
	for (QListIterator<eListboxEntry> entry(*top); i<entries; i++, ++entry)
		redrawEntry(target, i, entry.current(), where);
}

void eListbox::gotFocus()
{
	have_focus++;
	if (!top)
		return;
	QListIterator<eListboxEntry> entry(*top);
	for (int i=0; i<entries; i++, ++entry)
		if (entry.current()==current->current())
			invalidateEntry(i);
}

void eListbox::lostFocus()
{	
	have_focus--;
	if (!top)
		return;
	QListIterator<eListboxEntry> entry(*top);
	if (isVisible())
	{
		for (int i=0; i<entries; i++, ++entry)
			if (entry.current()==current->current())
				invalidateEntry(i);
	}

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");
}

void eListbox::actualize()
{
	if (current && !childs.containsRef(current->current()))
	{
		delete current;
		current=0;
	}
	if (!current)
		current=new QListIterator<eListboxEntry>(childs);
	if (top)
		delete top;
	if (bottom)
		delete bottom;
	entries=size.height()/item_height;
	top=new QListIterator<eListboxEntry>(childs);
	bottom=new QListIterator<eListboxEntry>(*top);
	(*bottom)+=entries;
	entries=size.height()/item_height;
}

void eListbox::redrawEntry(gPainter *target, int pos, eListboxEntry *entry, const eRect &where)
{
	eRect rect=eRect(ePoint(0, pos*item_height), eSize(size.width(), item_height));
	if (!where.contains(rect))
		return;

	gColor color=(have_focus && entry && entry==current->current())?col_active:getBackgroundColor();

	if(	entry && entry==current->current())
		/*emit*/ selchanged(*current);
	
	if (color != -1)
	{
		target->setForegroundColor(color);
		target->fill(rect);
		target->flush();
	}

	if (parent && parent->LCDElement && entry && (entry == current->current()))
		parent->LCDElement->setText(entry->getText(0));

	if (entry)
		entry->renderInto(target, rect);
	target->flush();
}

int eListbox::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_listActions->pageup)
			moveSelection(dirPageUp);
		else if (event.action == &i_listActions->pagedown)
			moveSelection(dirPageDown);
		else if (event.action == &i_cursorActions->up)
			moveSelection(dirUp);
		else if (event.action == &i_cursorActions->down)
			moveSelection(dirDown);
		else if (event.action == &i_cursorActions->ok)
		{
			if (!current)
				/*emit*/ selected(0);
			else
			{
				void* e = (void*)*current;
				if (*current)
					/*emit*/ current->current()->selected(*current);
				/*emit*/ selected(*current);
			}
		} else if (event.action == &i_cursorActions->cancel)
		{
			eDebug("cancel!");
			/*emit*/ selected(0);
		} else
			return 0;
		return 1;
	}
	return eWidget::eventHandler(event);
}

void eListbox::moveSelection(int dir)
{
	if (!current)
		return;
	int cs=1;
	
	eListboxEntry *oldptr=current->current(), *oldtop=top->current();

	switch (dir)
	{
	case dirPageDown:
		(*top)+=entries;
		(*bottom)+=entries;
		(*current)+=entries;
		if (!*top || !*current)
		{
			top->toLast();							// renew top
			(*top)-=entries-1;
			if (!*top)
				top->toFirst();
			current->toLast();
		}
		break;
	case dirPageUp:
		(*top)-=entries;
		(*bottom)-=entries;
		(*current)-=entries;
		if (!*top || !*current)
		{
			top->toFirst();
			current->toFirst();
		}
		break;
	case dirUp:
		
		if (current->atFirst())				// wrap around?
		{
			current->toLast();					// select last
			top->toLast();							// renew top
			(*top)-=entries-1;
			if (!*top)
				top->toFirst();
		} else
		{
			if (*current == *top)				// upper entry?
			{
				(*top)-=entries;				// renew top
				if (!*top)
					top->toFirst();
			}
			--(*current);								// go up
		}
		cs=1;
		break;
	case dirDown:
		if (current->atLast())				// wrap around?
		{
			current->toFirst();					// go to first
			top->toFirst();							// renew top
		} else
		{
			++(*current);
			if (*current == *bottom)
				*top=*bottom;
		}
		cs=1;
		break;
	}
	*bottom=*top;
	(*bottom)+=entries;
	if (isVisible())
	{
		if (oldtop!=top->current())
			invalidate();
		else if (cs)
		{
			int i=0;
			int old=-1, cur=-1;
			for (QListIterator<eListboxEntry> entry(*top); i<entries; i++, ++entry)
				if (entry.current()==oldptr)
					old=i;
				else if (entry.current()==current->current())
					cur=i;
				
			if (old != -1)
				invalidateEntry(old);
			if ((cur != -1) && (cur != old))
				invalidateEntry(cur);
		}
	}
}

void eListbox::setCurrent(eListboxEntry *c)
{
	if (!current)
		return;
	for (current->toFirst(); current->current(); ++*current)
		if (current->current()==c)
			break;
	if (!current->current())
		current->toFirst();
	if (bottom && top && entries)
	{
		QListIterator<eListboxEntry> it(*top);
		int i;
		for (i=0; i<entries; ++i, ++it)
			if (*it == *current)
				break;
		if (i == entries)
		{
			*top=*current;
			*bottom=*top;
			(*bottom)+=entries;
		}
	}
}

static eWidget *create_eListbox(eWidget *parent)
{
	return new eListbox(parent);
}

class eListboxSkinInit
{
public:
	eListboxSkinInit()
	{
		eSkin::addWidgetCreator("eListbox", create_eListbox);
	}
	~eListboxSkinInit()
	{
		eSkin::removeWidgetCreator("eListbox", create_eListbox);
	}
};

eAutoInitP0<eListboxSkinInit> init_eListboxSkinInit(3, "eListbox");
