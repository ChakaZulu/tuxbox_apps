#ifndef __elistBox_h
#define __elistBox_h

#include <core/driver/rc.h>
#include <core/gdi/grc.h>
#include <core/gdi/fb.h>
#include <core/gui/ewidget.h>
#include <core/gui/eskin.h>
#include <core/gui/ewindow.h>

template <class T>
class eListBox: public eWidget
{
	void redrawWidget(gPainter *target, const eRect &area);
	ePtrList<T> childs;
	ePtrList<T>::iterator top, bottom, current;

	int entries, font_size, item_height;
	gColor col_active;
	gFont entryFnt;

	void geometryChanged();
	void gotFocus();
	void lostFocus();
	eRect getEntryRect(int n);
	void invalidateEntry(int n);
public:
	void append(T* e);
	void remove(T* e);
	const gFont& getEntryFnt()	{		return entryFnt;	}
	eListBox(eWidget *parent, int FontSize=20);
	~eListBox();

	void keyDown(int rc);
	void keyUp(int rc);
	Signal1<void, T*> selected;	
	Signal1<void, T*> selchanged;

	void actualize();
	void clearList();
	void setCurrent(T *c);
	void sort();
	T* goNext();
	T* goPrev();
	int setProperty(const eString &prop, const eString &value);

	int have_focus;
	void setActiveColor(gColor active);
};

class eListBoxEntry: public Object
{
	friend class eListBox<eListBoxEntry>;
protected:
	eListBox<eListBoxEntry>* listBox;
public:
	eListBoxEntry(eListBox<eListBoxEntry>* parent)
		:listBox(parent)
	{	
		if (listBox)
			listBox->append(this);
	}
	~eListBoxEntry()
	{
		if (listBox)
			listBox->remove(this);
	}
};

class eListBoxEntryText: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryText>;
protected:
	eString text;
public:
	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const char* txt=0)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb), text(txt)
	{		
	
	}

	bool operator < ( const eListBoxEntryText& e) const
	{
		return text < e.text;	
	}

protected:
	void redraw(gPainter *rc, const eRect& rect, const gColor& coActive, const gColor& coNormal, bool highlited) const
	{
/*	if (parent && parent->LCDElement && entry && (entry == *current) )
		parent->LCDElement->setText( entry->getText(0) );*/
			rc->setForegroundColor(highlited?coActive:coNormal);
			rc->setFont(listBox->getEntryFnt());

			if ((coNormal != -1 && !highlited) || (highlited && coActive != -1))
					rc->fill(rect);

			rc->renderText(rect, text);
			rc->flush();
	}
};

class eListBoxEntryMenu: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryMenu>;
public:
	Signal0<void> selected;

	eListBoxEntryMenu(eListBox<eListBoxEntryMenu>* lb, const char* txt)
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, txt)
	{
		if (listBox)
			CONNECT(listBox->selected, eListBoxEntryMenu::LBSelected);
	}
	void LBSelected(eListBoxEntry* t)
	{
		if (t == this)
			/* emit */ selected();
	}
};

////////////////////////////////////// inline Methoden eListBox //////////////////////////////////////////
template <class T>
inline void eListBox<T>::append(T* entry)
{
	childs.push_back(entry);

//	if (auto_actualize)
	actualize();
}

template <class T>
inline void eListBox<T>::remove(T* entry)
{
	childs.take(entry);
//	if (auto_actualize)
		actualize();
}

template <class T>
inline void eListBox<T>::geometryChanged()
{
	entries=size.height()/item_height;

	if (!childs.empty())
	{
		bottom=top;
		bottom+=entries;
		if (bottom == childs.end())
			bottom--;
	}
}

template <class T>
inline void eListBox<T>::clearList()
{
	childs.clear();
}

template <class T>
inline void eListBox<T>::sort()
{
	childs.sort();
}

template <class T>
inline eRect eListBox<T>::getEntryRect(int pos)
{
	return eRect(ePoint(0, pos*item_height), eSize(size.width(), item_height));
}

template <class T>
inline void eListBox<T>::invalidateEntry(int n)
{
	invalidate(getEntryRect(n));
}

template <class T>
inline int eListBox<T>::setProperty(const eString &prop, const eString &value)
{
	if (prop=="col_active")
		col_active=eSkin::getActive()->queryScheme(value);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

template <class T>
inline void eListBox<T>::setActiveColor(gColor active)
{
	col_active=active;

	if (current != childs.end() && *current)
		invalidateEntry(active);		/* das ist ja wohl buggy hier */
}

template <class T>
inline T* eListBox<T>::goNext()
{
	keyDown(eRCInput::RC_DOWN);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline T* eListBox<T>::goPrev()
{
	keyDown(eRCInput::RC_UP);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline eListBox<T>::eListBox(eWidget *parent, int ih)
	 :eWidget(parent, 1),
		top(childs.end()), bottom(childs.end()), current(childs.end()),
		col_active(eSkin::getActive()->queryScheme("focusedColor")),
		font_size(ih),
		item_height(ih+2),
		have_focus(0),
		entryFnt(gFont("NimbusSansL-Regular Sans L Regular", font_size))
{
	childs.setAutoDelete(false);	// machen wir selber
}

template <class T>
inline eListBox<T>::~eListBox()
{
	while (childs.begin() != childs.end())
	{
		T* l=childs.front();
		delete l;
	}
}

template <class T>
inline void eListBox<T>::redrawWidget(gPainter *target, const eRect &where)
{
	ePtrList<T>::iterator entry(top);

	for (int i=0 ; have_focus && i < entries && entry != childs.end() ; i++, entry++)
	{
		eRect rect=eRect(ePoint(0, i*item_height), eSize(size.width(), item_height));

		if (!where.contains(rect))
			continue;

		entry->redraw(target, rect, col_active, getBackgroundColor(), *entry == *current);

		if (*entry == *current)  // than the item is the new highlited item
			/*emit*/ selchanged(*entry);
	}
}

template <class T>
inline void eListBox<T>::gotFocus()
{
	have_focus++;

	if (childs.empty())
		return;

	ePtrList<T>::iterator entry(top);

	for (int i=0; i<entries; i++, ++entry)
		if (*entry == *current)
			invalidateEntry(i);
}

template <class T>
inline void eListBox<T>::lostFocus()
{	
	have_focus--;

	if (childs.empty())
		return;

	ePtrList<T>::iterator entry(top);

	if (isVisible())
	{
		for (int i=0; i<entries; i++, ++entry)
			if (*entry == *current)
				invalidateEntry(i);
	}

/*	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");*/
}

template <class T>
inline void eListBox<T>::actualize()
{
	entries = size.height() / item_height;

	current = top = bottom = childs.begin();

	bottom += entries;

	if (bottom == childs.end())
		bottom--;
}

template <class T>
inline void eListBox<T>::keyDown(int rc)
{
	if (childs.empty())
		return;

	int cs=1;
	
	T *oldptr = *current,
		*oldtop = *top;

	switch (rc)
	{
		case eRCInput::RC_RIGHT:
			top += entries;
			bottom += entries;
			current += entries;
			if ( bottom == childs.end() )
			{
				bottom--; // bottom to last valid entry
				current = bottom; // current to last valid entry
				top = childs.end();
				top -= entries;		// top - entries
			}
		break;

		case eRCInput::RC_LEFT:
			top -= entries;
			bottom -= entries;
			current -= entries;
			if ( bottom == childs.begin() )
				bottom+=entries;
		break;
		
		case eRCInput::RC_UP:
			if ( current == childs.begin() )				// wrap around?
			{
				bottom = current = --childs.end();					// select last
				top = childs.end();
				top -= entries;
			}
			else
			{
				if (current == top)				// upper entry?
				{
					top -= entries;					// renew top   //???????????????????
					bottom -= entries;
				}
				current--;								// go up
			}
			cs=1;
		break;

		case eRCInput::RC_DOWN:
			if (current == --childs.end() )				// wrap around?
			{
				top = current = bottom = childs.begin(); 	// goto first;
				bottom += entries;
				if ( bottom == childs.end() )
					bottom--;
			}
			else
			{
				if (++current == bottom)
				{
					top = bottom;
					top -= entries;
				}
			}
			cs=1;
		break;
	}

	if (isVisible())
	{
		if (oldtop != *top)
			invalidate();
		else if (cs)
		{
			int i=0;
			int old=-1, cur=-1;
			for (ePtrList<T>::iterator entry(top); i<entries; i++, ++entry)
				if ( *entry == oldptr)
					old=i;
				else if ( *entry == *current )
					cur=i;
				
			if (old != -1)
				invalidateEntry(old);

			if ( (cur != -1) && (cur != old) )
				invalidateEntry(cur);
		}
	}
}

template <class T>
inline void eListBox<T>::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_HELP:
		/*emit*/ selected(0);
		return;
	case eRCInput::RC_OK:
		if ( current == childs.end() )
			/*emit*/ selected(0);
		else
			/*emit*/ selected(*current);
	}
}

template <class T>
inline void eListBox<T>::setCurrent(T *c)
{
	if (childs.empty())
		return;

	for (current == childs.begin(); current != childs.end() ; current++)
		if ( *current == c )
			break;

	if ( current == childs.end() )
		current = childs.begin();

	if (entries)
	{
		ePtrList<T>::iterator it(top);

		int i;

		for (i=0; i<entries; ++i, ++it)
			if (it == current)
				break;

		if (i == entries)
		{
			top=bottom=current;
			bottom+=entries;
		}
	}
}

template <class T>
class eListBoxWindow: public eWindow
{
protected:
	int Entrys;
	int width;
public:
	eListBox<T> list;
	eListBoxWindow(eString Title="", int Entrys=0, int FontSize=0, int width=400);
};

template <class T>
inline eListBoxWindow<T>::eListBoxWindow(eString Title, int Entrys, int FontSize, int width)
	: eWindow(0), Entrys(Entrys), width(width), list(this, FontSize)
{
	setText(Title);
	cresize(eSize(width, 10+Entrys*(FontSize+4)));
	
	list.move(ePoint(10, 5));
	eSize size = getClientSize();
	size.setWidth(size.width()-20);
	size.setHeight(size.height()-10);
	list.resize(size);
}

#endif
