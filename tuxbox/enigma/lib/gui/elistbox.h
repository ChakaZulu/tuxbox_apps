#ifndef __elistbox_h
#define __elistbox_h

#include "ewidget.h"
#include "rc.h"
#include "grc.h"
#include "eskin.h"

class eListbox;
#include <qsortedlist.h>

class eListboxEntry: public QObject
{
	Q_OBJECT

	friend class eListbox;
	eListbox *listbox;
	eListboxEntry *listboxentry;
	QSortedList<eListboxEntry> childs;
signals:
	void selected(eListboxEntry *lbe);
public:
	eListbox *parent;
	void *data;
	eListboxEntry(eListboxEntry *listboxentry, void *data=0);
	eListboxEntry(eListbox *listbox, void *data=0);
	
	virtual int operator<(const eListboxEntry &) const;
	virtual int operator==(const eListboxEntry &) const;
	virtual ~eListboxEntry();
	virtual QString getText(int col=0) const =0;
	virtual void renderInto(gPainter *rc, QRect rect) const;
};

class eListboxEntryText: public eListboxEntry
{
	QString text, sort;
public:
	QString getText(int col=0) const; 
	void setText(const QString t) { text=t; }
	eListboxEntryText(eListbox *listbox, QString text, QString sort=0, void *data=0);
	~eListboxEntryText();
};

class eListbox: public eWidget
{
	Q_OBJECT
	
	friend class eListboxEntry;
	void redrawWidget(gPainter *target, const QRect &area);
	QSortedList<eListboxEntry> childs;
	gFont entryFnt;
	QListIterator<eListboxEntry> *top, *bottom, *current;
	int entries, font_size, item_height;
	int type;
	
	gColor col_active;
	void actualize();
	void redrawEntry(gPainter *target, int pos, eListboxEntry *entry, const QRect &where);
	void geometryChanged();
	void gotFocus();
	void lostFocus();
	void OnFontSizeChanged(int NewFontSize);
	QRect getEntryRect(int n);
	void invalidateEntry(int n);
public:
	void keyDown(int rc);
	void keyUp(int rc);
signals:
	void selected(eListboxEntry *lbe);
	void selchanged(eListboxEntry *lbe);
public:
	void append(eListboxEntry *entry);
	void remove(eListboxEntry *entry);
	void clearList();
	void setCurrent(eListboxEntry *c);
	void sort();
	eListboxEntry *goNext();
	eListboxEntry *goPrev();
	int setProperty(const QString &prop, const QString &value);
	enum
	{
		tBorder, tLitebar
	};
	eListbox(eWidget *parent, int type=tLitebar, int FontSize=20);
	~eListbox();
	int have_focus;
	void setActiveColor(gColor active);
};

inline eListboxEntryText::eListboxEntryText(eListbox *listbox, QString text, QString sort, void *data)
:eListboxEntry(listbox, data) , text(text), sort(sort)
{
}

inline eListboxEntryText::~eListboxEntryText()
{
}

inline QString eListboxEntryText::getText(int col) const
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

inline eListboxEntry::eListboxEntry(eListboxEntry *listboxentry, void *data)
:listboxentry(listboxentry), data(data)
{
	parent=listboxentry->parent;
	listboxentry->childs.append(this);
	listbox=0;
}

inline eListboxEntry::eListboxEntry(eListbox *listbox, void *data)
:listbox(listbox), data(data)
{
	if (listbox)
	{
		parent=listbox;
		listboxentry=0;
		listbox->append(this);
	}
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

inline int eListboxEntry::operator<(const eListboxEntry &o) const
{
	return qstricmp(getText(-1), o.getText(-1))<0;
}

inline int eListboxEntry::operator==(const eListboxEntry &o) const
{
	return !qstricmp(getText(-1), o.getText(-1));
}

inline void eListboxEntry::renderInto(gPainter *rc, QRect area) const
{
	rc->setFont(listbox->entryFnt);
	rc->renderText(area, getText(0));
	rc->flush();
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

inline QRect eListbox::getEntryRect(int pos)
{
	return QRect(QPoint(0, pos*item_height), QSize(size.width(), item_height));
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

inline int eListbox::setProperty(const QString &prop, const QString &value)
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

#endif
