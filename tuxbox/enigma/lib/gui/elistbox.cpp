#include "elistbox.h"
#include "fb.h"
#include <qrect.h>
#include "elabel.h"
#include "init.h"

eListbox::eListbox(eWidget *parent, int type, int ih)
	 :eWidget(parent, 1), type(type)
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
}

void eListbox::redrawWidget(gPainter *target, const QRect &where)
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
	QListIterator<eListboxEntry> entry(*top);
	for (int i=0; i<entries; i++, ++entry)
		if (entry.current()==current->current())
			invalidateEntry(i);
}

void eListbox::lostFocus()
{	
	have_focus--;
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

void eListbox::redrawEntry(gPainter *target, int pos, eListboxEntry *entry, const QRect &where)
{
	QRect rect=QRect(QPoint(0, pos*item_height), QSize(size.width(), item_height));
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

void eListbox::keyDown(int rc)
{
	if (!current)
		return;
	int cs=1;
	
	eListboxEntry *oldptr=current->current(), *oldtop=top->current();

	switch (rc)
	{
	case eRCInput::RC_RIGHT:
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
	case eRCInput::RC_LEFT:
		(*top)-=entries;
		(*bottom)-=entries;
		(*current)-=entries;
		if (!*top || !*current)
		{
			top->toFirst();
			current->toFirst();
		}
		break;
		
	case eRCInput::RC_UP:
		
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
	case eRCInput::RC_DOWN:
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

void eListbox::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_HELP:
		/*emit*/ selected(0);
		return;
	case eRCInput::RC_OK:
		if (!current)
			/*emit*/ selected(0);
		else
		{
			void* e = (void*)*current;
			if (*current)
				/*emit*/ current->current()->selected(*current);
			/*emit*/ selected(*current);
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
		*top=*current;
		*bottom=*top;
		(*bottom)+=entries;
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
