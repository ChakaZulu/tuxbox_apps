#include "elistbox.h"
#include "fb.h"
#include "rc.h"
#include "eskin.h"
#include <qrect.h>
#include "elabel.h"

QString eListboxEntryText::getText(int col) const
{
	switch (col)
	{
	case -1:
		return sort?sort:text;
	case 0:
		return text;
	default:
		return 0;
	}
}

eListboxEntryText::eListboxEntryText(eListbox *listbox, QString text, QString sort, void *data): eListboxEntry(listbox, data) , text(text), sort(sort)
{
}

eListboxEntryText::~eListboxEntryText()
{
}

eListboxEntry::eListboxEntry(eListboxEntry *listboxentry, void *data): listboxentry(listboxentry), data(data)
{
	parent=listboxentry->parent;
	listboxentry->childs.append(this);
	listbox=0;
}

eListboxEntry::eListboxEntry(eListbox *listbox, void *data): listbox(listbox), data(data)
{
	parent=listbox;
	listboxentry=0;
	listbox->append(this);
}

eListboxEntry::~eListboxEntry()
{
	if (listboxentry)
		listboxentry->childs.remove(this);
	if (listbox)
		listbox->childs.remove(this);
	for (QListIterator<eListboxEntry> c(childs); c.current(); ++c)
		delete c.current();
}

int eListboxEntry::operator<(const eListboxEntry &o)
{
	return qstricmp(getText(-1), o.getText(-1))<0;
}

int eListboxEntry::operator==(const eListboxEntry &o)
{
	return !qstricmp(getText(-1), o.getText(-1));
}
 
void eListboxEntry::renderInto(gPainter *rc, QRect area) const
{
	rc->setFont(listbox->entryFnt);
	rc->renderText(area, getText(0));
	rc->flush();
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
				if (cur != -1)
					invalidateEntry(cur);
		}
	}
}

void eListbox::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_HELP:
		emit selected(0);
		return;
	case eRCInput::RC_OK:
		if (!current)
			emit selected(0);
		else
		{
			if (*current)
				emit current->current()->selected(*current);
			emit selected(*current);
		}
	}
}

void eListbox::append(eListboxEntry *entry)
{
	childs.append(entry);
	actualize();
}

void eListbox::remove(eListboxEntry *entry)
{
	childs.remove(entry);
	actualize();
}

void eListbox::geometryChanged()
{
	entries=size.height()/item_height;
	if (top)
	{
		bottom=new QListIterator<eListboxEntry>(*top);
		*bottom=*top;
		(*bottom)+=entries;
	}
}

void eListbox::clearList()
{
	childs.clear();
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

void eListbox::sort()
{
	childs.sort();
}

eListboxEntry *eListbox::goNext()
{
	keyDown(eRCInput::RC_DOWN);
	return current?current->current():0;
}

eListboxEntry *eListbox::goPrev()
{
	keyDown(eRCInput::RC_UP);
	return current?current->current():0;
}

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

void eListbox::OnFontSizeChanged(int NewFontSize)
{
	item_height=NewFontSize+2;
	font_size = NewFontSize;
	entryFnt=gFont("NimbusSansL-Regular Sans L Regular", font_size);
	geometryChanged();
}

QRect eListbox::getEntryRect(int pos)
{
	return QRect(QPoint(0, pos*item_height), QSize(size.width(), item_height));
}

void eListbox::invalidateEntry(int n)
{
	invalidate(getEntryRect(n));
}

eListbox::~eListbox()
{
	if (top)
		delete top;
	if (current)
		delete current;
}

int eListbox::setProperty(const QString &prop, const QString &value)
{
	if (prop=="col_active")
		col_active=eSkin::getActive()->queryScheme(value);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

void eListbox::setActiveColor(gColor active)
{
	col_active=active;
	if (current && current->current())
		invalidateEntry(active);
}
