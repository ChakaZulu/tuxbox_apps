#include <errno.h>

#include <enigma.h>
#include <lib/base/eptrlist.h>
#include <lib/base/eerror.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/actions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/guiactions.h>
#include <lib/system/init.h>

eWidget *eWidget::root;

eWidget::eWidget(eWidget *_parent, int takefocus):
	parent(_parent ? _parent : root),
	focus(0), takefocus(takefocus),
	font( parent ? parent->font : eSkin::getActive()->queryFont("global.normal") ),
	backgroundColor(_parent?gColor(-1):gColor(eSkin::getActive()->queryScheme("global.normal.background"))),
	foregroundColor(_parent?parent->foregroundColor:gColor(eSkin::getActive()->queryScheme("global.normal.foreground")))
{
	LCDTitle=0;
	LCDElement=0;
	LCDTmp=0;
	target=0;
	in_loop=0;
	if (parent && !(parent->state&stateVisible))
		state=stateShow;
	else
		state=0;

 	have_focus=0;
 	pixmap=0;
	if (takefocus)
		getTLW()->focusList()->push_back(this);

	if (parent)
		parent->childlist.push_back(this);
		
	just_showing=0;
}

eWidget::~eWidget()
{
	hide();
	if (takefocus)
		getTLW()->focusList()->remove(this);

	if (parent && !parent->childlist.empty())
		parent->childlist.remove(this);
	
	while (!childlist.empty())
		delete childlist.front();
}

void eWidget::takeFocus()
{
		// desktop shouldnt receive global focus
	ASSERT (parent);
		// childs shouldnt receive global focus
	ASSERT (!parent->parent);
	
	if (!have_focus)
	{
		oldTLfocus=eZap::getInstance()->focus;
		eZap::getInstance()->focus=this;
/*		if (oldTLfocus)
		{
			eDebug("focus problem");
			eFatal("da hat %s den focus und %s will ihn haben", oldTLfocus->getText().c_str(), getText().c_str());
		} */
		addActionMap(&i_focusActions->map);
	}
	have_focus++;
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
	 	have_focus--;
		if (!have_focus)
		{
			removeActionMap(&i_focusActions->map);
			if (eZap::getInstance()->focus==this)	// if we don't have lost the focus, ...
				eZap::getInstance()->focus=oldTLfocus;	// give it back
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
	size=nsize;
	recalcClientRect();
	event(eWidgetEvent(eWidgetEvent::changedSize));
	recalcClip();
}

void eWidget::move(const ePoint& nposition)
{
	position=nposition;
	event(eWidgetEvent(eWidgetEvent::changedPosition));
	recalcClip();
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
			eraseBackground(p, area);
			redrawWidget(p, area);
			delete p;
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
				It++;				
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

	// problem: überlappende, nicht transparente fenster

	while (force || (((int)w->getBackgroundColor())==-1))
	//	while (1)
	{
//		force=0;
		if (!w->parent)	// spaetestens fuers TLW sollte backgroundcolor aber non-transparent sein
			break;
		area.moveBy(w->position.x(), w->position.y());
		w=w->parent;
		area.moveBy(w->clientrect.x(), w->clientrect.y());
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

	event(eWidgetEvent(eWidgetEvent::execBegin));	// hat jemand was dagegen einzuwenden?

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
	if (in_loop==0)
		eFatal("attempt to close non-execing widget");
	if (in_loop==1)	// nur wenn das ne echte loop ist
		eApp->exit_loop();
	result=res;
}

void eWidget::show()
{
	if (state & stateShow)
		return;

	ASSERT(!(state&stateVisible));

	state|=stateShow;

	if (!parent || (parent->state&stateVisible))
	{
		getTLW()->just_showing++;
		willShowChildren();

		checkFocus();
		getTLW()->just_showing--;
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
			It++;
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
		checkFocus();
	}
	state&=~stateShow; 
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
			It++;
		}
	}
}

void eWidget::findAction(eActionPrioritySet &prio, const eRCKey &key, eWidget *context)
{
	for (actionMapList::iterator i = actionmaps.begin(); i != actionmaps.end(); ++i)
		(*i)->findAction(prio, key, context);
}

int eWidget::eventFilter(const eWidgetEvent &event)
{
	return 0;
}

int eWidget::eventHandler(const eWidgetEvent &evt)
{
	switch (evt.type)
	{
	case eWidgetEvent::childChangedHelpText:
		/* emit */ focusChanged(focus);  // faked focusChanged Signal to the Statusbar
	break;
	case eWidgetEvent::evtAction:
		if (evt.action == &i_focusActions->up)
			focusNext(focusDirPrev);
		else if (evt.action == &i_focusActions->down)
			focusNext(focusDirNext);
		else if (evt.action == &i_focusActions->left)
			focusNext(focusDirPrev);
		else if (evt.action == &i_focusActions->right)
			focusNext(focusDirNext);
		else
			return 0;
		return 1;
	case eWidgetEvent::evtKey:
	{
		eActionPrioritySet prio;

		findAction(prio, *evt.key, this);
		if (focus && (focus != this))
			focus->findAction(prio, *evt.key, focus);

		// and look at global ones. NOT YET.
		
		for (eActionPrioritySet::iterator i(prio.begin()); i != prio.end(); ++i)
		{
			if (i->first)
			{
				if (((eWidget*)i->first)->event(eWidgetEvent(eWidgetEvent::evtAction, i->second)))
					break;
			} else
			{
				(const_cast<eAction*>(evt.action))->handler();	// only useful for global actions
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
	case eWidgetEvent::changedText:
	case eWidgetEvent::changedFont:
	case eWidgetEvent::changedForegroundColor:
	case eWidgetEvent::changedBackgroundColor:
	case eWidgetEvent::changedPosition:
	case eWidgetEvent::changedPixmap:
		invalidate();
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
	clientrect=eRect(0, 0, size.width(), size.height());
}

void eWidget::recalcClip()
{
	eWidget *t=this;
	eRect rect=eRect(0, 0, size.width(), size.height());
	while (t)
	{
		rect&=t->clientrect;
		rect.moveBy(t->position.x(), t->position.y());
		t=t->parent;
		if (t)
			rect.moveBy(t->clientrect.x(), t->clientrect.y());
	}
	clientclip=rect;
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

void eWidget::redrawWidget(gPainter *target, const eRect &clip)
{
}

void eWidget::eraseBackground(gPainter *target, const eRect &clip)
{
	if (((int)getBackgroundColor())!=-1)
	{
		target->clear();
		target->flush();
	}
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
					tries--;
				}
			} else if (dir == focusDirPrev)
			{
				if (_focusList.current() == _focusList.begin())
				{
					_focusList.last();
					tries--;
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
			if (!i->state&stateVisible)
				continue;
			ePoint m1=i->getPosition();
			m1+=ePoint(i->getSize().width()/2, i->getSize().height()/2);
			ePoint m2=_focusList.current()->getPosition();
			m2+=ePoint(_focusList.current()->getSize().width()/2, _focusList.current()->getSize().height()/2);
			
			int xd=m1.x()-m2.x();
			int yd=m1.y()-m2.y();
#if 0
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
#else
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

void eWidget::setBackgroundColor(const gColor& color)
{
	if (color!=backgroundColor)
	{
		backgroundColor=color;
		event(eWidgetEvent(eWidgetEvent::changedBackgroundColor));
	}
}

void eWidget::setForegroundColor(const gColor& color)
{
	if (color != foregroundColor)
	{
		foregroundColor=color;
		event(eWidgetEvent(eWidgetEvent::changedForegroundColor));
	}
}

void eWidget::setPixmap(gPixmap *pmap)
{
	pixmap=pmap;
	event(eWidgetEvent(eWidgetEvent::changedPixmap));
}

void eWidget::setTarget(gDC *newtarget)
{
	target=newtarget;
}

void eWidget::setLCD(eWidget *_lcdtitle, eWidget *_lcdelement)
{
	LCDTitle=_lcdtitle;
	LCDElement=_lcdelement;
}

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
	while (r && !r->target)
		r = r->parent;
		
	ASSERT(r);
	ASSERT(r->target);

	gPainter *p=new gPainter(*r->target, myclip);
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
			p++;
			ea=1;
		}

		char *x;
		v[i]=strtol(p, &x, 10);
		p=x;

		if (*p && *p!=':')
			 return -3;

		if (*p==':')
			p++;

		if (ea)
			v[i]+=e[i];

		i++;
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
		setPixmap(eSkin::getActive()->queryImage(value));
	else if (prop=="foregroundColor")
		setForegroundColor(eSkin::getActive()->queryColor(value));
	else if (prop=="backgroundColor")
		setBackgroundColor(eSkin::getActive()->queryColor(value));
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
			It++;
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
			crect.setRight( width() - 1 - deco.borderRight );
			crect.setBottom( height() - 1 - deco.borderBottom );
		}
		if (deco_selected)
		{
			crect_selected.setLeft( deco_selected.borderLeft );
			crect_selected.setTop( deco_selected.borderTop );
			crect_selected.setRight( width() - 1 - deco_selected.borderRight );
			crect_selected.setBottom( height() - 1 - deco_selected.borderBottom );
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

eAutoInitP0<eWidgetSkinInit> init_eWidgetSkinInit(3, "eWidget");
