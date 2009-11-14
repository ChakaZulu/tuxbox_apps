#include <errno.h>

#include <lib/base/eptrlist.h>
#include <lib/base/eerror.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/actions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/guiactions.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

#include <lib/gui/echeckbox.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/textinput.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/enumber.h>
#include <lib/gui/elabel.h>
#include <lib/gui/combobox.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/slider.h>

extern eWidget *currentFocus;

eWidget *eWidget::root;
Signal2< void, ePtrList<eAction>*, int >eWidget::showHelp;
Signal1< void, const eWidget*>eWidget::globalFocusChanged;
eWidget::actionMapList eWidget::globalActions;

eWidget::eWidget(eWidget *_parent, int takefocus)
	:helpID(0), parent(_parent ? _parent : root),
	shortcut(0), shortcutFocusWidget(0),
	focus(0), TLW(0), takefocus(takefocus),
	state( parent && !(parent->state&stateVisible) ? stateShow : 0 ),
	target(0), result(0), in_loop(0), have_focus(0), just_showing(0),
	font( parent ? parent->font : eSkin::getActive()->queryFont("global.normal") ),
	backgroundColor(_parent?gColor(-1):gColor(eSkin::getActive()->queryScheme("global.normal.background"))),
	foregroundColor(_parent?parent->foregroundColor:gColor(eSkin::getActive()->queryScheme("global.normal.foreground"))),
	pixmap(0)
#ifndef DISABLE_LCD
	,LCDTitle(0)
	,LCDElement(0)
	,LCDTmp(0)
#endif
{
	init_eWidget();
}
void eWidget::init_eWidget()
{
	if (takefocus)
		getTLW()->focusList()->push_back(this);

	if (parent)
		parent->childlist.push_back(this);

	addActionMap(&i_cursorActions->map);
}
eWidget::~eWidget()
{
	if (pixmap)
		pixmap->compressdata();
	hide();
	if (takefocus)
	{
		getTLW()->focusList()->remove(this);
		if (getTLW()->focus == this)
			eFatal("focus still held.");
	}
		
	if (shortcut)
		getTLW()->actionListener.remove(this);

	if (parent && !parent->childlist.empty())
		parent->childlist.remove(this);
	
	while (!childlist.empty())
		delete childlist.front();
}

void eWidget::setActive( bool active, eWidget *insert, bool after )
{
	eWidget *tlw = getTLW();
	if (active && !takefocus)
	{
		ePtrList<eWidget> &list = *getTLW()->focusList();
		if ( insert )
		{
			ePtrList<eWidget>::iterator it =
				std::find( list.begin(), list.end(), insert );
			if ( it != list.end() )
				list.insert( after ? ++it : it, this );
			else
				list.push_back(this);
		}
		else
			list.push_back(this);
		if ( tlw->isVisible() )
			tlw->takeFocus();
		takefocus=1;
	}
	else if (!active && takefocus)
	{
		if ( have_focus )
			lostFocus();
		if ( tlw->isVisible() )
			tlw->releaseFocus();
		tlw->focusList()->remove(this);
		takefocus=0;
	}
}

void eWidget::takeFocus()
{
		// desktop shouldnt receive global focus
	ASSERT (parent);
		// childs shouldnt receive global focus
	ASSERT (!parent->parent);
	
	if (!have_focus)
	{
		oldTLfocus=currentFocus;
		currentFocus=this;
		/*emit*/ globalFocusChanged(currentFocus);
/*		if (oldTLfocus)
		{
			eDebug("focus problem");
			eFatal("da hat %s den focus und %s will ihn haben", oldTLfocus->getText().c_str(), getText().c_str());
		} */
		addActionMap(&i_focusActions->map);
	}
	++have_focus;
}

void eWidget::releaseFocus()
{
		// desktop shouldnt receive global focus
	ASSERT (parent);
		// childs shouldnt receive global focus
	ASSERT (!parent->parent);
	ASSERT (have_focus);

	if (have_focus)
	{
	 	--have_focus;
		if (!have_focus)
		{
			removeActionMap(&i_focusActions->map);
			if (currentFocus==this)	// if we don't have lost the focus, ...
			{
				currentFocus=oldTLfocus;	// give it back
				/*emit*/ globalFocusChanged(currentFocus);
			}
			else
				eFatal("someone has stolen the focus");
		}
 	}
}

void eWidget::_willShow()
{
	ASSERT(state&stateShow);
	ASSERT(!(state&stateVisible));
	state|=stateVisible;
	if (takefocus)
		getTLW()->takeFocus();
	willShow();
}

void eWidget::_willHide()
{
	ASSERT(state&stateShow);
	ASSERT(state&stateVisible);
	state&=~stateVisible;

	willHide();
	if (takefocus)
		getTLW()->releaseFocus();
}

void eWidget::willShow()
{
	event(eWidgetEvent(eWidgetEvent::willShow));
}

void eWidget::willHide()
{
	event(eWidgetEvent(eWidgetEvent::willHide));
}

void eWidget::setPalette()
{
}

void eWidget::resize(const eSize& nsize)
{
	bool b = size != nsize;
	size=nsize;
	recalcClientRect();
	if ( b )
		event(eWidgetEvent(eWidgetEvent::changedSize));
	recalcClip();
}

void eWidget::recalcAbsolutePosition()
{
	absPosition = (parent?(parent->getAbsolutePosition()+parent->clientrect.topLeft()+position):position);
	for (ePtrList<eWidget>::iterator it( childlist ); it != childlist.end(); ++it )
		it->recalcAbsolutePosition();
}

void eWidget::move(const ePoint& nposition)
{
	bool b = position != nposition;
	position=nposition;
	recalcAbsolutePosition();
	recalcClip();
	if ( b )
		event(eWidgetEvent(eWidgetEvent::changedPosition));
}

void eWidget::cresize(const eSize& nsize)
{
	recalcClientRect();
	resize(eSize(nsize.width()+size.width()-clientrect.width(), nsize.height()+size.height()-clientrect.height()));
}

void eWidget::cmove(const ePoint& nposition)
{
	recalcClientRect();
	move(ePoint(nposition.x()-clientrect.x(), nposition.y()-clientrect.y()));
}

void eWidget::valign()
{ 
	unsigned int v_tvsystem;
	eConfig::getInstance()->getKey("/elitedvb/video/tvsystem", v_tvsystem );
	
	if ( v_tvsystem==2)
	{
		move(ePoint((720-size.width())/2, (480-size.height())/2 )); // NTSC: 720x480
  	}
	else 
	{
		move(ePoint((720-size.width())/2, (576-size.height())/2)); // PAL: 720x576
  	}
}

void eWidget::redraw(eRect area)		// area bezieht sich nicht auf die clientarea
{
	if (getTLW()->just_showing)
		return;

	if (state & stateVisible )
	{
		if (area.isNull())
			area=eRect(0, 0, size.width(), size.height());
		if (area.width()>0)
		{
			gPainter *p=getPainter(area);
			if (p)
			{
				eraseBackground(p, area);
				redrawWidget(p, area);
				delete p;
			}
		}
		if(!childlist.empty())
		{
			area.moveBy(-clientrect.x(), -clientrect.y());		// ab hier jetzt schon.

			ePtrList<eWidget>::iterator It(childlist);
			while (It != childlist.end())
			{
				eRect cr=area&eRect((*It)->position, (*It)->size);
				if (!cr.isEmpty())
				{
					cr.moveBy(-It->position.x(), -It->position.y());
					It->redraw(cr);
				}
				++It;
			}
		}
	}
}

void eWidget::invalidate(eRect area, int force)
{
	if ( (!(state & stateVisible)) && (!force))
		return;

	if (area.isNull())
		area=eRect(0, 0, size.width(), size.height());

	eWidget *w=this;

	// problem: �berlappende, nicht transparente fenster

	while (force || (((int)w->getBackgroundColor())==-1))
	//	while (1)
	{
		force=0;
		if (!w->parent)	// spaetestens fuers TLW sollte backgroundcolor aber non-transparent sein
			break;
		area.moveBy(w->position.x(), w->position.y());
		w=w->parent;
		area.moveBy(w->clientrect.x(), w->clientrect.y());
		area&=w->clientrect;
	}
	w->redraw(area);
}

int eWidget::event(const eWidgetEvent &event)
{
	if (!eventFilter(event))
	{
		eWidget *target=this;
/*		if (have_focus && event.toFocus())	// bypassing focus handling for root-widget
			target=focusList()->current();  */
		if (target)
		{
			while (target)
			{
				if (target->eventHandler(event))
					return 1;
				if (target==this)
					break;
				target=target->parent;
			} 
		}
	}
	return 0;
}

	/* das ist bestimmt ne einzige race hier :) */
int eWidget::exec()
{
	if (in_loop)
		eFatal("double exec");

	in_loop=-1;	// hey, exec hat angefangen aber noch nicht in der mainloop

	event(eWidgetEvent(eWidgetEvent::execBegin)); // hat jemand was dagegen einzuwenden?
	if (in_loop)	// hatte wohl jemand.
	{
		in_loop=1;		// wir betreten die mainloop
		eApp->enter_loop();		// oder wir machen das halt selber.
		in_loop=0; // nu sind wir jedenfalls draussen.
	}
	event(eWidgetEvent(eWidgetEvent::execDone));
	return result;
}

void eWidget::clear()
{
#if 0
	eWidget *root=this;
	while (root->parent)
		root=root->parent;
	eRect me(getRelativePosition(root), size);
	root->invalidate(me);
#endif
	invalidate(eRect(), 1);
}

void eWidget::close(int res)
{
	event(eWidgetEvent(eWidgetEvent::wantClose,res));
}

void eWidget::show()
{
	if (state & stateShow)
		return;

	ASSERT(!(state&stateVisible));

	state|=stateShow;

	if (!parent || (parent->state&stateVisible))
	{
		++getTLW()->just_showing;
		willShowChildren();

		checkFocus();
		--getTLW()->just_showing;
		redraw();
	}
}


void eWidget::accept()
{
	close(0);
}

void eWidget::reject()
{
	close(-1);
}

void eWidget::willShowChildren()
{
	if (!(state & stateShow))
		return;
	_willShow();
	if (!childlist.empty())
	{
		ePtrList<eWidget>::iterator It(childlist);
		while(It != childlist.end())
		{
			It->willShowChildren();
			++It;
		}
	}
}

void eWidget::hide()
{
	if (!(state&stateShow))
		return;
	
	if (state&stateVisible)
	{
		willHideChildren();
		clear();	// hide -> immer erasen. dieses Hide ist IMMER explizit.
	}
	state&=~stateShow; 
	checkFocus();
}

void eWidget::willHideChildren()
{
	if (!(state & stateShow))
		return;
	_willHide();
	if (!childlist.empty())
	{
		ePtrList<eWidget>::iterator It(childlist);
		while(It != childlist.end())
		{
			It->willHideChildren();
			++It;
		}
	}
}

void eWidget::findAction(eActionPrioritySet &prio, const eRCKey &key, eWidget *context)
{
	for (actionMapList::iterator i = actionmaps.begin(); i != actionmaps.end(); ++i)
	{
		const std::set<eString> &styles=eActionMapList::getInstance()->getCurrentStyles();
		for (std::set<eString>::const_iterator si(styles.begin()); si != styles.end(); ++si)
			(*i)->findAction(prio, key, context, *si);
	}

	for(ePtrList<eWidget>::iterator w(actionListener.begin()); w != actionListener.end(); ++w)
		i_shortcutActions->map.findAction(prio, key, w, "");

	if (focus && focus != this)
		focus->findAction(prio, key, context);
}

int eWidget::eventFilter(const eWidgetEvent &event)
{
	return 0;
}

void eWidget::addActionToHelpList(eAction *action)
{
	actionHelpList.push_back( action );
}

void eWidget::clearHelpList()
{
	actionHelpList.clear();
}

void eWidget::setHelpID(int fHelpID)
{
	helpID=fHelpID;
}

int eWidget::eventHandler(const eWidgetEvent &evt)
{
	switch (evt.type)
	{
	case eWidgetEvent::childChangedHelpText:
		/* emit */ focusChanged(focus);  // faked focusChanged Signal to the Statusbar
		break;
	case eWidgetEvent::evtAction:
		if (evt.action == shortcut && isVisible())
			(shortcutFocusWidget?shortcutFocusWidget:this)->
				event(eWidgetEvent(eWidgetEvent::evtShortcut));
		else if (evt.action == &i_focusActions->up)
			focusNext(focusDirPrev);
		else if (evt.action == &i_focusActions->down)
			focusNext(focusDirNext);
		else if (evt.action == &i_focusActions->left)
			focusNext(focusDirPrev);
		else if (evt.action == &i_focusActions->right)
			focusNext(focusDirNext);
		else if (evt.action == &i_cursorActions->help)
		{
			int wasvisible=state&stateShow;
			if (wasvisible)
				hide();
			/* emit */ showHelp( &actionHelpList, helpID );
			if (wasvisible)
				show();
		} else
			return 0;
		return 1;
	case eWidgetEvent::evtKey:
	{
		eActionPrioritySet prio;

		findAction(prio, *evt.key, this);

		if (focus && (focus != this))
			focus->findAction(prio, *evt.key, focus);

		for (actionMapList::iterator i = globalActions.begin(); i != globalActions.end(); ++i)
		{
			const std::set<eString> &styles=eActionMapList::getInstance()->getCurrentStyles();
			for (std::set<eString>::const_iterator si(styles.begin()); si != styles.end(); ++si)
				(*i)->findAction(prio, *evt.key, 0, *si);
		}
		
		for (eActionPrioritySet::iterator i(prio.begin()); i != prio.end(); ++i)
		{
			if (i->first)
			{
				if (((eWidget*)i->first)->event(eWidgetEvent(eWidgetEvent::evtAction, i->second)))
					break;
			} else
			{
				(const_cast<eAction*>(i->second))->handler();	// only useful for global actions
				break;
			}
		}

		if (focus)
		{
			/* Action not found, try to use old Keyhandle */
			int c = evt.key->producer->getKeyCompatibleCode(*evt.key);
			if (c != -1)
			{
				if (evt.key->flags & eRCKey::flagBreak)
					focus->keyUp(c);
				else
					focus->keyDown(c);
			}
		}
		return 1;
		break;
	}
	case eWidgetEvent::gotFocus:
		gotFocus();
		break;
	case eWidgetEvent::lostFocus:
		lostFocus();
		break;
	case eWidgetEvent::changedSize:
	case eWidgetEvent::changedFont:
	case eWidgetEvent::changedPosition:
	case eWidgetEvent::changedPixmap:
		invalidate();
		break;
	case eWidgetEvent::evtShortcut:
			setFocus(this);
		break;
	case eWidgetEvent::wantClose:
/*		if (in_loop==0)
			eFatal("attempt to close non-execing widget");*/
		if (in_loop==1)	// nur wenn das ne echte loop ist
		{
			in_loop=-1;
			eApp->exit_loop();
		}
		result=evt.parameter;
		break;
	default:
		break;
	}
	return 0;
}

int eWidget::keyDown(int rc)
{
	return 0;
}

int eWidget::keyUp(int rc)
{
	return 0;
}

void eWidget::gotFocus()
{
}

void eWidget::lostFocus()
{
}

void eWidget::recalcClientRect()
{
	clientrect.setWidth(size.width());
	clientrect.setHeight(size.height());
}

void eWidget::recalcClip()
{
	eWidget *t=this;
	clientclip=eRect(0, 0, size.width(), size.height());
	while (t)
	{
		clientclip&=t->clientrect;
		clientclip.moveBy(t->position.x(), t->position.y());
		t=t->parent;
		if (t)
			clientclip.moveBy(t->clientrect.x(), t->clientrect.y());
	}
	for (ePtrList<eWidget>::iterator it( childlist ); it != childlist.end(); ++it )
		it->recalcClip();
}

void eWidget::checkFocus()
{
	ePtrList<eWidget> *l=getTLW()->focusList();
	if (!(getTLW()->focus && getTLW()->focus->state&stateVisible))
	{
		l->first();

		while (l->current() && !(l->current()->state&stateVisible))
			l->next();

		setFocus(l->current());
	}
}

void eWidget::addActionMap(eActionMap *map)
{
	actionmaps.push_back(map);
}

void eWidget::removeActionMap(eActionMap *map)
{
	actionmaps.remove(map);
}

void eWidget::addGlobalActionMap(eActionMap *map)
{
	globalActions.push_back(map);
}

void eWidget::removeGlobalActionMap(eActionMap *map)
{
	globalActions.remove(map);
}

void eWidget::redrawWidget(gPainter *target, const eRect &clip)
{
}

void eWidget::eraseBackground(gPainter *target, const eRect &clip)
{
	if (((int)getBackgroundColor()) >= 0)
		target->clear();
}

void eWidget::focusNext(int dir)
{
	if (parent && parent->parent)
		return getTLW()->focusNext(dir);

	if (!_focusList.current())
		_focusList.first();
	if (!_focusList.current())
	{
		setFocus(0);
		return;
	}

	switch (dir)
	{
	case focusDirNext:
	case focusDirPrev:
	{
		int tries=2;
		while (tries)
		{
			if (dir == focusDirNext)
			{
				_focusList.next();
				if (!_focusList.current())
				{
					_focusList.first();
					--tries;
				}
			} else if (dir == focusDirPrev)
			{
				if (_focusList.current() == _focusList.begin())
				{
					_focusList.last();
					--tries;
				} else
					_focusList.prev();
			}
			if (_focusList.current() && _focusList.current()->state&stateVisible)
				break;
		}
		if (!tries)
		{
			setFocus(0);
			return;
		}
		break;
	}
	case focusDirN:
	case focusDirE:
	case focusDirS:
	case focusDirW:
	{
		eWidget *nearest=_focusList.current();
		int difference=1<<30;
		for (ePtrList<eWidget>::iterator i(_focusList.begin()); i != _focusList.end(); ++i)
		{
			if (_focusList.current() == i)
				continue;
			if (!(i->state&stateVisible))
				continue;
			ePoint m1=i->getAbsolutePosition();
			ePoint m2=_focusList.current()->getAbsolutePosition();
			switch (dir)
			{
			case focusDirN:		// diff between our TOP and new BOTTOM
				m1+=ePoint(i->getSize().width()/2, i->getSize().height());
				m2+=ePoint(_focusList.current()->getSize().width()/2, 0);
				break;
			case focusDirS:		// diff between our BOTTOM and new TOP
				m1+=ePoint(i->getSize().width()/2, 0);
				m2+=ePoint(_focusList.current()->getSize().width()/2, _focusList.current()->getSize().height());
				break;
			case focusDirE:		// diff between our RIGHT and new LEFT border
				m1+=ePoint(0, i->getSize().height()/2);
				m2+=ePoint(_focusList.current()->getSize().width(), _focusList.current()->getSize().height()/2);
				break;
			case focusDirW:		// diff between our LEFT and new RIGHT border
				m1+=ePoint(i->getSize().width(), i->getSize().height()/2);
				m2+=ePoint(0, _focusList.current()->getSize().height()/2);
				break;
			}
			
			int xd=m1.x()-m2.x();
			int yd=m1.y()-m2.y();
#define METHOD 0
#if METHOD == 0
			int dif=xd*xd+yd*yd;
			int eff=0;

			switch (dir)
			{
			case focusDirN:
				yd=-yd;
			case focusDirS:
				if (yd > 0)
					eff=dif/yd;
				else
					eff=1<<30;
				break;
			case focusDirW:
				xd=-xd;
			case focusDirE:
				if (xd > 0)
					eff=dif/xd;
				else
					eff=1<<30;
				break;
			}

			if (eff < difference)
			{
				difference=eff;
				nearest=*i;
			}
#elif METHOD == 1
			int ldir=focusDirN;
			int mydiff=0;

			if (xd > mydiff)	// rechts
			{
				mydiff=xd;
				ldir=focusDirE;
			}
			if ((-xd) > mydiff) // links
			{
				mydiff=-xd;
				ldir=focusDirW;
			}
			if (yd > mydiff)		// unten
			{
				mydiff=yd;
				ldir=focusDirS;
			}
			if ((-yd) > mydiff)	// oben
			{
				mydiff=-yd;
				ldir=focusDirN;
			}
			if (dir == ldir)	// nur elemente beruecksichtigen die in der richtung liegen...
			{
				int entf=xd*xd+yd*yd;
				if (entf < difference)
				{
					difference=entf;
					nearest=*i;
				}
			}
#elif METHOD == 2

#endif

		}
		_focusList.setCurrent(nearest);
		break;
	}
	}

	setFocus(_focusList.current());
}

void eWidget::setFocus(eWidget *newfocus)
{
	if (parent && parent->parent)
		return getTLW()->setFocus(newfocus);

	if (focus == newfocus)
		return;

	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::lostFocus));

	focus=newfocus;

	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::gotFocus));

	_focusList.setCurrent(focus);

	/* emit */ focusChanged(focus);
}

void eWidget::setHelpText( const eString& e)
{
	helptext=e;
	if ( parent )
		parent->event( eWidgetEvent::childChangedHelpText );
}

void eWidget::setFont(const gFont &fnt)
{
	font=fnt;
	event(eWidgetEvent(eWidgetEvent::changedFont));
}

void eWidget::setText(const eString &label)
{
	if (label != text)	// ein compare ist immer weniger arbeit als ein unnoetiges redraw
	{
		text=label;
		event(eWidgetEvent(eWidgetEvent::changedText));
	}
}

void eWidget::setBackgroundColor(const gColor& color, bool inv)
{
	if (color!=backgroundColor)
	{
		backgroundColor=color;
		event(eWidgetEvent(eWidgetEvent::changedBackgroundColor));
		if (inv)
			invalidate();
	}
}

void eWidget::setForegroundColor(const gColor& color, bool inv)
{
	if (color != foregroundColor)
	{
		foregroundColor=color;
		event(eWidgetEvent(eWidgetEvent::changedForegroundColor));
		if (inv)
			invalidate();
	}
}

void eWidget::setPixmap(gPixmap *pmap)
{
	if ( pixmap != pmap )
	{
		pixmap=pmap;
		event(eWidgetEvent(eWidgetEvent::changedPixmap));
	}
}

void eWidget::setTarget(gDC *newtarget)
{
	target=newtarget;
}

#ifndef DISABLE_LCD
void eWidget::setLCD(eWidget *_lcdtitle, eWidget *_lcdelement)
{
	LCDTitle=_lcdtitle;
	LCDElement=_lcdelement;
}
#endif

void eWidget::setName(const char *_name)
{
	name=_name;
}

gPainter *eWidget::getPainter(eRect area)
{
	eRect myclip=eRect(getAbsolutePosition(), size);
	if (parent)
		myclip&=parent->clientclip;

	eWidget *r=this;
	while (r && r->parent && !r->target)
		r = r->parent;

	ASSERT(r);
//	ASSERT(r->target);
	if (!r->target)	// if target is 0, device is locked.
		return 0;

	gPainter *p=new gPainter(*r->target, myclip);
	p->setLogicalZero(getAbsolutePosition());
	if (!area.isNull())
		p->clip(area);
	p->setForegroundColor(foregroundColor);
	p->setBackgroundColor(backgroundColor);
	return p;
}

static int parse(const char* p, int *v, int *e, int max)
{
	int i=0;

	while ( (i<max) && (*p) )
	{
		int ea=0;

		if (*p=='e')
		{
			++p;
			ea=1;
		}

		char *x;
		v[i]=strtol(p, &x, 10);
		p=x;

		if (*p && *p!=':')
			 return -3;

		if (*p==':')
			++p;

		if (ea)
			v[i]+=e[i];

		++i;
	}

	if (*p)
		return -1;

	if (i<max)
		return -2;

	return 0;
}

int eWidget::setProperty(const eString &prop, const eString &value)
{
	if (prop=="position")
	{
		int v[2], e[2]={0, 0};
		if (parent)
		{
			e[0]=parent->clientrect.width();
			e[1]=parent->clientrect.height();
		}
		int err=parse(value.c_str(), v, e, 2);
		if (err)
			return err;
		move(ePoint(v[0], v[1]));
	}
	else if (prop=="cposition")
	{
		int v[2], e[2];
		e[0]=e[1]=0;
		if (parent)
		{
			e[0]=parent->clientrect.width();
			e[1]=parent->clientrect.height();
		}
		int err=parse(value.c_str(), v, e, 2);
		if (err)
			return err;

		cmove(ePoint(v[0], v[1]));
	}
	else if (prop=="size")
	{
		int v[2], e[2];
		e[0]=e[1]=0;
		if (parent)
		{
			e[0]=parent->clientrect.width()-position.x();
			e[1]=parent->clientrect.height()-position.y();
		}
		int err=parse(value.c_str(), v, e, 2);
		if (err)
			return err;
		resize(eSize(v[0], v[1]));
	}
	else if (prop=="csize")
	{
		int v[2], e[2];
		e[0]=e[1]=0;
		if (parent)
		{
			e[0]=parent->clientrect.width()-position.x();
			e[1]=parent->clientrect.height()-position.y();
		}
		int err=parse(value.c_str(), v, e, 2);
		if (err)
			return err;
		cresize(eSize(v[0], v[1]));
	}
	else if (prop=="text")
/*	{
		eString text;

		std::string::const_iterator p(value.begin());

		while(*p)
		{
			if (*p=='\\')
			{
				switch (*(++p))
				{
				case 'n':
					text+='\n';
					break;
				case 'r':
					text+='\r';
					break;
				case 't':
					text+='\t';
					break;
				case 'b':
					text+='\b';
					break;
				case '\\':
					text+='\\';
					break;
				default:
					text+='?';
					break;
				}
			}
			else
				text+=*p;

			p++;
		}
		setText(text);
	}*/
		setText(::gettext(value.c_str()));
	else if (prop=="helptext")
		setHelpText(::gettext(value.c_str()));
	else if (prop=="font")
		setFont(eSkin::getActive()->queryFont(value));
	else if (prop=="name")
		name=value;
	else if (prop=="pixmap")
	{
		setPixmap(eSkin::getActive()->queryImage(value));
		if (pixmap)
			pixmap->uncompressdata();

	}
	else if (prop=="foregroundColor")
		setForegroundColor(eSkin::getActive()->queryColor(value));
	else if (prop=="backgroundColor")
		setBackgroundColor(eSkin::getActive()->queryColor(value));
	else if (prop=="shortcut")
		setShortcut(value);
	else if (prop=="shortcutFocus")
		setShortcutFocus(parent ? parent->search(value) : 0);
	else
	{
		eFatal("skin property %s does not exist", prop.c_str());
		return -ENOENT;
	}
	return 0;
}

eWidget *eWidget::search(const eString &sname)
{
	if (name==sname)
		return this;

	if (!childlist.empty())
	{
		std::list<eWidget*>::iterator It = childlist.begin();
		while(It != childlist.end())
		{
			eWidget* p = (*It)->search(sname);
			if (p)
				return p;
			++It;
		}
	}
	return 0;
}

void eWidget::makeRoot()
{
	root=this;
}

void eWidget::zOrderLower()
{
	if (!parent)
		return;
	int isshown=0;
	if (state & stateShow)
	{
		isshown=1;
		hide();
	}
	parent->childlist.remove(this);
	parent->childlist.push_front(this);
	if (isshown)
		show();
}

void eWidget::zOrderRaise()
{
	if (!parent)
		return;
	int isshown=0;
	if (state & stateShow)
	{
		isshown=1;
		hide();
	}
	parent->childlist.remove(this);
	parent->childlist.push_back(this);
	if (isshown)
		show();
}

void eWidget::setShortcut(const eString &shortcutname)
{
	if (shortcut)
		getTLW()->actionListener.remove(this);
	shortcut=i_shortcutActions->map.findAction(shortcutname.c_str());
	if (shortcut)
		getTLW()->actionListener.push_back(this);
}

void eWidget::setShortcutFocus(eWidget *focus)
{
	shortcutFocusWidget=focus;
	if (!focus)
		eFatal("setShortcutFocus with unknown widget!");
}

eProgress* eWidget::CreateSkinnedProgress(const char* name, int start, int perc, int takefocus )
{
	eProgress* prg = new eProgress(this, takefocus);
	prg->setName(name);
	prg->setParams( start, perc );
	return prg;
}
eSlider* eWidget::CreateSkinnedSlider(const char* name, const char *descr, int min, int max )
{
	eSlider* s = new eSlider(this,(descr ? CreateSkinnedLabel(descr) : 0), min, max);
	s->setName(name);
	return s;

}
eLabel* eWidget::CreateSkinnedLabel(const char* name,const char* text, int flags)
{
	eLabel* l = new eLabel(this,flags);
	l->setName(name);
	if (text)
		l->setText(text);
	return l;
}
eComboBox* eWidget::CreateSkinnedComboBoxWithLabel(const char* name, int OpenEntries, const char* lbldescr, int takefocus)
{
	return  CreateSkinnedComboBox(name, OpenEntries, (lbldescr ? CreateSkinnedLabel(lbldescr) : 0), takefocus);
}
eComboBox* eWidget::CreateSkinnedComboBox(const char* name, int OpenEntries, eLabel* descr, int takefocus)
{
	eComboBox* cbo = new eComboBox(this, OpenEntries, descr, takefocus );
	cbo->setName(name);
	return cbo;
}

eCheckbox* eWidget::CreateSkinnedCheckbox(const char* name,int defaultvalue, const char* configkey,int takefocus)
{
	if (configkey)
		eConfig::getInstance()->getKey(configkey, defaultvalue );
	eCheckbox* chk =new eCheckbox(this,defaultvalue,takefocus);
	chk->setName(name);
	return chk;
}
eNumber* eWidget::CreateSkinnedNumberWithLabel(const char* name, int defaultvalue, int len, int min, int max, int maxdigits, int *init, int isactive, const char* lbldescr, int grabfocus)
{
	return CreateSkinnedNumber(name, defaultvalue, len, min, max, maxdigits, init, isactive, (lbldescr ? CreateSkinnedLabel(lbldescr) : 0), grabfocus);
}
eNumber* eWidget::CreateSkinnedNumber(const char* name, int defaultvalue, int len, int min, int max, int maxdigits, int *init, int isactive, eLabel* descr, int grabfocus)
{
	eNumber* n = new eNumber(this, len, min, max, maxdigits, init, isactive, descr, grabfocus);
	n->setName(name);
	if (!init)
		n->setNumber(defaultvalue);
	return n;
}
eTextInputField* eWidget::CreateSkinnedTextInputField(const char* name, const char* defaultvalue, const char* configkey, const char* lblname,eTextInputFieldHelpWidget *hlp)
{
	eTextInputField* tb=new eTextInputField(this,lblname ? CreateSkinnedLabel(lblname) : 0,hlp);
	tb->setName(name);
	if (defaultvalue)
	{
		eString val = eString(defaultvalue);
		char* p = 0;
		if (configkey)
			eConfig::getInstance()->getKey(configkey, p);
		if (p) val = eString(p);
		tb->setText(val);
	}
	return tb;
}
eButton* eWidget::CreateSkinnedButton(const char* name, eLabel* descr, int takefocus)
{
	eButton* btn =new eButton(this, descr, takefocus);
	btn->setName(name);
	return btn;
}
void eWidget::BuildSkin(const char* name)
{
	if (eSkin::getActive()->build(this, name))
		eFatal("skin load of \"%s\" failed",name);
};

static eWidget *create_eWidget(eWidget *parent)
{
	return new eWidget(parent);
}

class eWidgetSkinInit
{
public:
	eWidgetSkinInit()
	{
		eSkin::addWidgetCreator("eWidget", create_eWidget);
	}
	~eWidgetSkinInit()
	{
		eSkin::removeWidgetCreator("eWidget", create_eWidget);
	}
};

int eDecoWidget::setProperty( const eString &prop, const eString &value)
{
	if (prop == "loadDeco")
	{
		if ( value != "" )
			strDeco=value;

		loadDeco();
	}
	else
		return eWidget::setProperty( prop, value );

	return 0;
}

int eDecoWidget::eventFilter( const eWidgetEvent &evt )
{
	if ( evt.type == eWidgetEvent::changedSize )
	{
		if (deco)
		{
			crect.setLeft( deco.borderLeft );
			crect.setTop( deco.borderTop );
			crect.setWidth( width() - (deco.borderRight + deco.borderLeft) );
			crect.setHeight( height() - (deco.borderBottom + deco.borderTop ) );
		}
		if (deco_selected)
		{
			crect_selected.setLeft( deco_selected.borderLeft );
			crect_selected.setTop( deco_selected.borderTop );
			crect_selected.setWidth( width() - (deco_selected.borderRight + deco_selected.borderLeft) );
			crect_selected.setHeight( height() - (deco_selected.borderBottom + deco_selected.borderTop ) );
		}
	}
	return 0; //always return 0... the eventHandler must been called...
}

void eDecoWidget::loadDeco()
{
	int i = 0;
	if ( deco.load( strDeco ) )
		i |= 1;
	if ( deco_selected.load( strDeco+".selected" ) )
		i |= 1;
	if (i)
		event( eWidgetEvent::changedSize );
}

eAutoInitP0<eWidgetSkinInit> init_eWidgetSkinInit(eAutoInitNumbers::guiobject, "eWidget");
