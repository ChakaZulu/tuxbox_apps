#ifndef __elistbox_h
#define __elistbox_h

#include "ewidget.h"
#include "grc.h"

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
	
	int operator<(const eListboxEntry &);
	int operator==(const eListboxEntry &);
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

#endif
