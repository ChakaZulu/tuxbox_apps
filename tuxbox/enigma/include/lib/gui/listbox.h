#ifndef __listbox_h
#define __listbox_h

#include <sstream>

#include <lib/driver/rc.h>
#include <lib/gdi/grc.h>
#include <lib/gdi/fb.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/statusbar.h>

int calcFontHeight( const gFont& font );

class eListBoxBase: public eDecoWidget
{
	gPixmap *iArrowUpDown, *iArrowUp, *iArrowDown, *iArrowLeft, *iArrowRight;
protected:
	const eWidget* descr;
	eLabel* tmpDescr; // used for description Label in LCD
	gColor colorActiveB, colorActiveF;
	eRect crect, crect_selected;
	enum  { arNothing, arCurrentOld, arAll};
	int MaxEntries, item_height, flags, columns, in_atomic, atomic_redraw, atomic_old, atomic_new;
	bool atomic_selchanged;
	int movemode;
public:
	enum	{		flagNoUpDownMovement=1,		flagNoPageMovement=2,		flagShowEntryHelp=4	};
	enum	{		OK = 0,		ERROR=1,		E_ALLREADY_SELECTED = 2,		E_COULDNT_FIND = 4,		E_INVALID_ENTRY = 8,	 E_NOT_VISIBLE = 16		};
	void setFlags(int);
	void removeFlags(int);
	void invalidateEntry(int n){	invalidate(getEntryRect(n));}
	void invalidateContent();
	void setColumns(int col);
	int getColumns() { return columns; }
	void setMoveMode(int move) { movemode=move; }
protected:
	eListBoxBase(eWidget* parent, const eWidget* descr=0, const char *deco="eListBox" );
	eRect getEntryRect(int n);
	int setProperty(const eString &prop, const eString &value);
	int eventHandler(const eWidgetEvent &event);
	void recalcMaxEntries();
	void recalcClientRect();
	void redrawBorder(gPainter *target, eRect &area);
	int newFocus();
	void gotFocus();
	void lostFocus();
};

template <class T>
class eListBox: public eListBoxBase
{
	typedef typename ePtrList<T>::iterator ePtrList_T_iterator;
	ePtrList<T> childs;
	ePtrList_T_iterator top, bottom, current;
	int recalced;
	void redrawWidget(gPainter *target, const eRect &area);
	int eventHandler(const eWidgetEvent &event);
	void lostFocus();
	void gotFocus();
public:
	eListBox(eWidget *parent, const eWidget* descr=0 );
	~eListBox();

	void init();

	void append(T* e, bool holdCurrent=false);
	void remove(T* e, bool holdCurrent=false);
	void clearList();
	int getCount() { return childs.size(); }

	Signal1<void, T*> selected;	
	Signal1<void, T*> selchanged;

	int setCurrent(const T *c);
	T* getCurrent()	{ return current != childs.end() ? *current : 0; }
	T *getNext() { ePtrList_T_iterator c=current; ++c; return c != childs.end() ? *c : 0; }
	T* goNext();
	T* goPrev();

	void sort();

	template <class Z>
	int forEachEntry(Z ob)
	{
		for (ePtrList_T_iterator i(childs.begin()); i!=childs.end(); ++i)
			if ( ob(**i) )
				return OK;

		return ERROR;
	}

	template <class Z>
	int forEachVisibleEntry(Z ob)
	{
		if (!isVisible())
			return E_NOT_VISIBLE;

		for (ePtrList_T_iterator i(top); i!=bottom; ++i)
			if ( ob(**i) )
				return OK;

		return ERROR;
	}

	void invalidateCurrent()
	{
		int n=0;
		for (ePtrList_T_iterator i(top); i != bottom; ++i, n++)
			if ( i == current )
				invalidate(getEntryRect(n));    
	}
  
	enum
	{
		dirPageDown, dirPageUp, dirDown, dirUp, dirFirst
	};

	int moveSelection(int dir);
	void setActiveColor(gColor back, gColor front);

	void beginAtomic();
	void endAtomic();
};

class eListBoxEntry: public Object
{
	friend class eListBox<eListBoxEntry>;
protected:
	eListBox<eListBoxEntry>* listbox;
	eString helptext;
public:
	eListBoxEntry(eListBox<eListBoxEntry>* parent, const char *hlptxt=0)
		:listbox(parent), helptext(hlptxt?hlptxt:_("no description avail"))
	{	
		if (listbox)
			listbox->append(this);
	}
	virtual ~eListBoxEntry()
	{
		if (listbox)
			listbox->remove(this);
	}

	void drawEntryRect( gPainter* rc, const eRect& where, const gColor& coActiveB, const gColor& coActiveF, const gColor& coNormalB, const gColor& coNormalF, int state );
	const eString &getHelpText() const { return helptext; }
};

class eListBoxEntryText: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryText>;
protected:
	eString text;
	void *key;
	int align;
	eTextPara *para;
	int yOffs;
	static gFont font;
	int keytype;
public:
	enum { value, ptr };
	static int getEntryHeight();

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const char* txt=0, void *key=0, int align=0, const char* hlptxt=0, int keytype = value )
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb, hlptxt ), text(txt),
		 key(key), align(align), para(0), keytype(keytype)
	{
	}

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const eString& txt, void* key=0, int align=0, const char* hlptxt=0, int keytype = value )
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb, hlptxt ), text(txt),
		 key(key), align(align), para(0), keytype(keytype)
	{
	}

	~eListBoxEntryText();
	
	bool operator < ( const eListBoxEntryText& e) const
	{
		if (key == e.key || keytype == ptr)
			return text < e.text;	
		else
			return key < e.key;
	}
	
	void *& getKey() { return key; }
	const void* getKey() const { return key; }
	const eString& getText() { return text; }
protected:
	const eString& redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryTextStream: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTextStream>;
protected:
	std::stringstream text;
	static gFont font;
public:
	static int getEntryHeight();

	eListBoxEntryTextStream(eListBox<eListBoxEntryTextStream>* lb)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb)
	{		
	}

	bool operator < ( const eListBoxEntryTextStream& e) const
	{
		return text.str() < e.text.str();	
	}

protected:
	eString redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryMenu: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryMenu>;
public:
	Signal0<void> selected;

	eListBoxEntryMenu(eListBox<eListBoxEntryMenu>* lb, const char* txt, const char* hlptxt=0, int align=0 )
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, txt, 0, align, hlptxt)
	{
		if (listbox)
			CONNECT(listbox->selected, eListBoxEntryMenu::LBSelected);
	}
	void LBSelected(eListBoxEntry* t)
	{
		if (t == this)
			/* emit */ selected();
	}
};

////////////////////////////////////// inline Methoden eListBox //////////////////////////////////////////


template <class T>
inline void eListBox<T>::append(T* entry, bool holdCurrent)
{
	T* cur = 0;
	if (holdCurrent)
		cur = current;

	childs.push_back(entry);
	init();
	
	if (cur)
		setCurrent(cur);
}

template <class T>
inline void eListBox<T>::remove(T* entry, bool holdCurrent)
{
	T* cur = 0;

	if (holdCurrent && current != entry)
		cur = current;

	childs.take(entry);
	init();

	if (cur)
		setCurrent(cur);
}

template <class T>
inline void eListBox<T>::clearList()
{
	while (!childs.empty())
		delete childs.first();
	current = top = bottom = childs.end();
	if (!in_atomic)
	{
		selchanged(0);
		invalidateContent();
	} else
	{
		atomic_selchanged=1;
		atomic_redraw=arAll;
	}
}

template <class T>
inline void eListBox<T>::sort()
{
	T* cur = current;
	childs.sort();

	init();

	if (cur)
		setCurrent(cur);
}

template <class T>
inline T* eListBox<T>::goNext()
{
	moveSelection(dirDown);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline T* eListBox<T>::goPrev()
{
	moveSelection(dirUp);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline eListBox<T>::eListBox(eWidget *parent, const eWidget* descr)
	:eListBoxBase(parent, descr),
		top(childs.end()), bottom(childs.end()), current(childs.end()), recalced(0)
{
	childs.setAutoDelete(false);	// machen wir selber

	addActionMap(&i_cursorActions->map);
	addActionMap(&i_listActions->map);
	item_height = T::getEntryHeight();
}

template <class T>
inline eListBox<T>::~eListBox()
{
	while (childs.begin() != childs.end())
		delete childs.front();
}

template <class T>
inline void eListBox<T>::redrawWidget(gPainter *target, const eRect &where)
{
	eRect rc = where;

	eListBoxBase::redrawBorder(target, rc);

	// rc wird in eListBoxBase ggf auf den neuen Client Bereich ohne Rand verkleinert

	int i=0;
	for (ePtrList_T_iterator entry(top); (entry != bottom) && (entry != childs.end()); ++entry)
	{
		eRect rect = getEntryRect(i);

		eString s;

		if ( rc.contains(rect) )
		{
			target->clip(rect);
			if ( entry == current )
			{
				if ( LCDTmp ) // LCDTmp is only valid, when we have the focus
					LCDTmp->setText( entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), 1 ) );				
				else if ( parent->LCDElement && have_focus )
					parent->LCDElement->setText( entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), 1 ) );
				else
					entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), ( have_focus ? 1 : ( MaxEntries > 1 ? 2 : 0 ) )	);		
			}
			else
				entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), 0 /*( have_focus ? 0 : ( MaxEntries > 1 ? 2 : 0 ) )*/	);
			target->clippop();
		}

		i++;
	}

	target->flush();
}

template <class T>
inline void eListBox<T>::gotFocus()
{
	eListBoxBase::gotFocus();

	have_focus++;

	if (!childs.empty())
		if ( eListBoxBase::newFocus() )   // recalced ?
		{
			ePtrList_T_iterator it = current;
			init();	
			setCurrent(it);
		}
		else if ( isVisible() )
		{
			int i=0;	
			for (ePtrList_T_iterator entry(top); entry != bottom; ++i, ++entry)
				if (entry == current)
					invalidateEntry(i);
		}
}

template <class T>
inline void eListBox<T>::lostFocus()
{	
	eListBoxBase::lostFocus();

	have_focus--;

	if (!childs.empty())
		if ( eListBoxBase::newFocus() ) 	//recalced ?
		{
			ePtrList_T_iterator it = current;
			init();	
			setCurrent(it);
		}
		else if ( isVisible() )
		{
			int i = 0;
			for (ePtrList_T_iterator entry(top); entry != bottom; i++, ++entry)
				if (entry == current)
					invalidateEntry(i);
		}

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");

}

template <class T>
inline void eListBox<T>::init()
{
	current = top = bottom = childs.begin();

	for (int i = 0; i < (MaxEntries*columns); ++i, ++bottom)
		if (bottom == childs.end() )
			break;	
	if (!in_atomic)
		invalidateContent();
	else
		atomic_redraw=arAll;
}

template <class T>
inline int eListBox<T>::moveSelection(int dir)
{
	int direction=0, forceredraw=0;

	if (childs.empty())
		return 0;

	ePtrList_T_iterator oldptr=current, oldtop=top;

	switch (dir)
	{
		case dirPageDown:
			direction=+1;
			for (int i = 0; i < MaxEntries; i++)
			{
				if (++current == bottom) // unten (rechts) angekommen? page down
				{
					if (bottom == childs.end()) // einzige ausnahme: unten (rechts) angekommen
					{
						--current;
						break;
					}
					for (int i = 0; i < MaxEntries * columns; ++i)
					{
						if (bottom != childs.end())
						{
							++bottom;
							++top;
						}
					}
				}
			}
		break;

		case dirPageUp:
			direction=-1;
			for (int i = 0; i < MaxEntries; ++i)
			{
				if (current == childs.begin())
					break;

				if (current-- == top/* && current != childs.begin()*/ )	// oben (links) angekommen? page up
				{
					for (int i = 0; i < MaxEntries * columns; ++i)
					{
						if (--top == childs.begin()) 		// einzige ausnahme: oben (links) angekommen
							break;
					}

					// und einmal bottom neuberechnen :)
					bottom=top;
					for (int i = 0; i < MaxEntries*columns; ++i)
						if (bottom != childs.end())
							++bottom;
				}
			}
			break;
		
		case dirUp:
			if ( current == childs.begin() )				// wrap around?
			{
				direction=+1;
				current = childs.end();					// select last
				--current;
				top = bottom = childs.end();
				for (int i = 0; i < MaxEntries*columns; i++, top--)
					if (top == childs.begin())
						break;
			} else
			{
				direction=-1;
				if (current-- == top) // new top must set
				{
					for (int i = 0; i < MaxEntries*columns; i++, top--)
						if (top == childs.begin())
							break;
					bottom=top;
					for (int i = 0; i < MaxEntries*columns; ++i, ++bottom)
						if (bottom == childs.end())
							break;
				}
			}
		break;

		case dirDown:
			if ( current == --ePtrList_T_iterator(childs.end()) )				// wrap around?
			{
				direction=-1;
				top = current = bottom = childs.begin(); 	// goto first
				for (int i = 0; i < MaxEntries * columns; ++i, ++bottom)
					if ( bottom == childs.end() )
						break;
			}
			else
			{
				direction=+1;
				if (++current == bottom)   // ++current ??
				{
					for (int i = 0; i<MaxEntries * columns; ++i)
					{
						if (bottom != childs.end() )
							++bottom;
						if (top != childs.end() )
							++top;
					}
				}
			}
			break;
		case dirFirst:
			direction=-1;
			top = current = bottom = childs.begin(); 	// goto first;
			for (int i = 0; i < MaxEntries * columns; i++, bottom++)
				if ( bottom == childs.end() )
					break;
			break;
		default:
			return 0;
	}

	if (current != oldptr)  // current has changed
	{
		if (movemode)
		{
				// feel free to rewrite using stl::copy[_backward], but i didn't succeed.
			typename std::list<T*>::iterator o=oldptr;
			typename std::list<T*>::iterator c=current;
			typename std::list<T*>::iterator curi=current;
			typename std::list<T*>::iterator oldi=oldptr;
			int count=0;

			T* old=*o;

			if (direction > 0)
			{
				++o;
				++c;
				while (o != c)
				{
					*oldi++=*o++;
					count++;
				}
			} else
			{
				while (o != curi)
				{
					*oldi--=*--o;
					count++;
				}
			}
			
			if (count > 1)
				forceredraw=1;
			
			*curi=old;
		}

		if (!in_atomic)
			selchanged(*current);
		else
			atomic_selchanged=1;
	}

	if (flags & flagShowEntryHelp)
	{
		setHelpText( current != childs.end() ? current->getHelpText():eString(_("no description available")));
	}

	if (isVisible())
	{
		if ((oldtop != top) || forceredraw)
		{
			if (in_atomic)
				atomic_redraw=arAll;
			else
				invalidateContent();
		} else if ( current != oldptr)
		{
			int i=0;
			int old=-1, cur=-1;
			
			for (ePtrList_T_iterator entry(top); entry != bottom; i++, ++entry)
				if ( entry == oldptr)
					old=i;
				else if ( entry == current )
					cur=i;
			
			if (in_atomic)
			{
				if (atomic_redraw == arNothing)
				{
					atomic_old=old;
					atomic_redraw = arCurrentOld;
				}
				if (atomic_redraw == arCurrentOld)
					atomic_new=cur;
			} else
			{
				if (old != -1)
					invalidateEntry(old);
				if (cur != -1)
					invalidateEntry(cur);
			}
		}
	}
	return 1;
}

template <class T>
inline int eListBox<T>::eventHandler(const eWidgetEvent &event)
{
	eListBoxBase::eventHandler(event);  // this calls not eWidget::eventhandler...

	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			init();
		break;

		case eWidgetEvent::evtAction:
			if ((event.action == &i_listActions->pageup) && !(flags & flagNoPageMovement))
				moveSelection(dirPageUp);
			else if ((event.action == &i_listActions->pagedown) && !(flags & flagNoPageMovement))
				moveSelection(dirPageDown);
			else if ((event.action == &i_cursorActions->up) && !(flags & flagNoUpDownMovement))
				moveSelection(dirUp);
			else if ((event.action == &i_cursorActions->down) && !(flags & flagNoUpDownMovement))
				moveSelection(dirDown);
			else if (event.action == &i_cursorActions->ok)
			{
				if ( current == childs.end() )
					/*emit*/ selected(0);
				else
					/*emit*/ selected(*current);
			}
			else if (event.action == &i_cursorActions->cancel)
				/*emit*/ selected(0);
			else
				break;
		return 1;
		default:
		break;
	}
	return eWidget::eventHandler(event);
}

template <class T>
inline int eListBox<T>::setCurrent(const T *c)
{
	if (childs.empty() || ((current != childs.end()) && (*current == c)))  // no entries or current is equal the entry to search
		return E_ALLREADY_SELECTED;	// do nothing

	ePtrList_T_iterator item(childs.begin()), it(childs.begin());

	for ( ; item != childs.end(); item++)
		if ( *item == c )
			break;

	if ( item == childs.end() ) // entry not in listbox... do nothing
		return E_COULDNT_FIND;

	int newCurPos=-1;
	int oldCurPos=-1;
	ePtrList_T_iterator oldCur(current);

	int i = 0;

	for (it=top; it != bottom; ++it, ++i)  // check if entry to set between bottom and top
	{
		if (it == item)
		{
			newCurPos=i;
			current = it;
		}
		if ( it == oldCur)
			oldCurPos=i;
	}

	if (newCurPos != -1)	// found on current screen, so redraw only old and new
	{
		if (isVisible())
		{
			if (in_atomic)
			{
				if (atomic_redraw == arNothing)
				{
					atomic_redraw = arCurrentOld;
					atomic_old = oldCurPos;
				}
				if (atomic_redraw == arCurrentOld)
					atomic_new = newCurPos;
			} else
			{
				invalidateEntry(newCurPos);
				if (oldCurPos != -1)
					invalidateEntry(oldCurPos);
			}
		}
	}	else // the we start to search from begin
	{
		bottom = childs.begin();

		while (newCurPos == -1 && MaxEntries )  // MaxEntries is already checked above...
		{
			if ( bottom != childs.end() )
				top = bottom;		// nächster Durchlauf

			for (	i = 0; (i < (MaxEntries*columns) ) && (bottom != childs.end()); ++bottom, ++i)
			{
				if (bottom == item)
				{
					current = bottom;  // we have found
					newCurPos++;
				}
      }
		}
		if (isVisible())
		{
			if (!in_atomic)
				invalidateContent();   // Draw all
			else
				atomic_redraw=arAll;
		}
  }

	if (!in_atomic)
		selchanged(*current);
	else
		atomic_selchanged=1;

	return OK;
}

template <class T>
void eListBox<T>::setActiveColor(gColor back, gColor front)
{
	colorActiveB=back;
	colorActiveF=front;

	if (current != childs.end())
	{
		int i = 0;
		for (ePtrList_T_iterator it(top); it != bottom; i++, it++)
		{
			if (it == current)
			{
				invalidateEntry(i);
				break;
			}
		}
	}
}

template <class T>
void eListBox<T>::beginAtomic()
{
	if (!in_atomic++)
	{
		atomic_redraw=arNothing;
		atomic_selchanged=0;
		atomic_new=-1;
	}
}

template <class T>
void eListBox<T>::endAtomic()
{
	if (!--in_atomic)
	{
		if (atomic_redraw == arAll)
			invalidateContent();
		else if (atomic_redraw == arCurrentOld)
		{
			if (atomic_new != -1)
				invalidateEntry(atomic_new);
			if (atomic_old != -1)
				invalidateEntry(atomic_old);
		}
		if (atomic_selchanged)
			if (childs.empty())
				selchanged(0);
			else
				selchanged(*current);
	}
}

template <class T>
class eListBoxWindow: public eWindow
{
protected:
	int Entrys;
	int width;
	eListBox<T> list;
	eStatusBar *statusbar;
public:
	eListBoxWindow(eString Title="", int Entrys=0, int width=400, bool sbar=0);
};

template <class T>
inline eListBoxWindow<T>::eListBoxWindow(eString Title, int Entrys, int width, bool sbar)
	: eWindow(0), Entrys(Entrys), width(width), list(this), statusbar(sbar?new eStatusBar(this):0)
{
	list.setFlags( eListBoxBase::flagShowEntryHelp );
	setText(Title);
	cresize( eSize(width, (sbar?40:10)+Entrys*T::getEntryHeight() ) );
	list.move(ePoint(10, 5));
	list.resize(eSize(getClientSize().width()-20, getClientSize().height()-(sbar?35:5) ));
	if (sbar)
	{
		statusbar->setFlags(eStatusBar::flagVCenter);
		statusbar->move( ePoint(0, getClientSize().height()-30) );
		statusbar->resize( eSize( getClientSize().width(), 30) );
		statusbar->loadDeco();
		statusbar->show();
	}
}

#endif
