#ifndef __elistbox_h
#define __elistbox_h

#include "ewidget.h"
#include "rc.h"
#include "grc.h"
#include "eskin.h"

class eListbox;
#include <qsortedlist.h>

class eListboxEntry: public Object
{
	friend class eListbox;
protected:
	eListbox *listbox;
public:
	Signal1<void, eListboxEntry*> selected;
	void *data;
	eListboxEntry(eListbox *listbox, void *data=0);
	
	virtual int operator<(const eListboxEntry &) const;
	virtual int operator==(const eListboxEntry &) const;
	virtual ~eListboxEntry();
	virtual eString getText(int col=0) const =0;
	virtual void renderInto(gPainter *rc, eRect rect) const;
};

class eListboxEntryText: public eListboxEntry
{
	eString text, sort;
public:
	eString getText(int col=0) const; 
	void setText(const eString t) { text=t; }
	eListboxEntryText(eListbox *listbox, const char* text, const char* sort=0, void *data=0);
	~eListboxEntryText();
};

class eListbox: public eWidget
{
	friend class eListboxEntry;
	void redrawWidget(gPainter *target, const eRect &area);
	QSortedList<eListboxEntry> childs;
	gFont entryFnt;
	QListIterator<eListboxEntry> *top, *bottom, *current;
	int entries, font_size, item_height;
	
	gColor col_active;
	void actualize();
	void redrawEntry(gPainter *target, int pos, eListboxEntry *entry, const eRect &where);
	void geometryChanged();
	void gotFocus();
	void lostFocus();
	void OnFontSizeChanged(int NewFontSize);
	eRect getEntryRect(int n);
	void invalidateEntry(int n);
public:
	void keyDown(int rc);
	void keyUp(int rc);
	Signal1<void, eListboxEntry*> selected;
	Signal1<void, eListboxEntry*> selchanged;
public:
	void append(eListboxEntry *entry);
	void remove(eListboxEntry *entry);
	void clearList();
	void setCurrent(eListboxEntry *c);
	eListboxEntry *getCurrent()
	{
		if (!current)
			return 0;
		return *current;
	}
	void sort();
	eListboxEntry *goNext();
	eListboxEntry *goPrev();
	int setProperty(const eString &prop, const eString &value);
	eListbox(eWidget *parent, int FontSize=20);
	~eListbox();
	int have_focus;
	void setActiveColor(gColor active);
};

inline eListboxEntry::eListboxEntry(eListbox *listbox, void *data)
:listbox(listbox), data(data)
{
	if (listbox)
		listbox->append(this);
}

inline eListboxEntry::~eListboxEntry()
{
	if (listbox)
		listbox->childs.remove(this);
}

inline int eListboxEntry::operator<(const eListboxEntry &o) const
{
	return getText(-1).icompare(o.getText(-1)) < 0;
}

inline int eListboxEntry::operator==(const eListboxEntry &o) const
{
	return !getText(-1).icompare(o.getText(-1));
}

inline void eListboxEntry::renderInto(gPainter *rc, eRect area) const
{
	rc->setFont(listbox->entryFnt);
	rc->renderText(area, getText(0));
	rc->flush();
}

inline eListboxEntryText::eListboxEntryText(eListbox *listbox, const char* text, const char* sort, void *data)
:eListboxEntry(listbox, data), text(text), sort(sort)
{
}

inline eListboxEntryText::~eListboxEntryText()
{
}

inline eString eListboxEntryText::getText(int col) const
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

inline void eListbox::append(eListboxEntry *entry)
{
	childs.append(entry);
	actualize();
}

inline void eListbox::remove(eListboxEntry *entry)
{
	childs.remove(entry);
	actualize();
}

inline void eListbox::geometryChanged()
{
	entries=size.height()/item_height;
	if (top)
	{
		bottom=new QListIterator<eListboxEntry>(*top);
		*bottom=*top;
		(*bottom)+=entries;
	}
}

inline void eListbox::clearList()
{
	childs.clear();
}

inline void eListbox::sort()
{
	childs.sort();
}

inline void eListbox::OnFontSizeChanged(int NewFontSize)
{
	item_height=NewFontSize+2;
	font_size = NewFontSize;
	entryFnt=gFont("NimbusSansL-Regular Sans L Regular", font_size);
	geometryChanged();
}

inline eRect eListbox::getEntryRect(int pos)
{
	return eRect(ePoint(0, pos*item_height), eSize(size.width(), item_height));
}

inline void eListbox::invalidateEntry(int n)
{
	invalidate(getEntryRect(n));
}

inline eListbox::~eListbox()
{
	if (top)
		delete top;
	if (current)
		delete current;
}

inline int eListbox::setProperty(const eString &prop, const eString &value)
{
	if (prop=="col_active")
		col_active=eSkin::getActive()->queryScheme(value);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

inline void eListbox::setActiveColor(gColor active)
{
	col_active=active;

	if (current && current->current())
		invalidateEntry(active);		/* das ist ja wohl buggy hier */
}

inline eListboxEntry *eListbox::goNext()
{
	keyDown(eRCInput::RC_DOWN);
	return current?current->current():0;
}

inline eListboxEntry *eListbox::goPrev()
{
	keyDown(eRCInput::RC_UP);
	return current?current->current():0;
}

#endif
