#include <errno.h>

#include "enigma.h"
#include <core/base/eerror.h>
#include <core/gui/ewidget.h>
#include <core/gdi/gfbdc.h>
#include <core/gdi/epng.h>
#include <core/gui/eskin.h>
#include <core/system/init.h>
#include <core/gui/actions.h>
#include <core/base/eptrlist.h>
#include "guiactions.h"

eWidget::eWidget(eWidget *parent, int takefocus):
	parent(parent),
	takefocus(takefocus), 
	font(parent?parent->font:gFont("NimbusSansL-Regular Sans L Regular", eSkin::getActive()->queryValue("fontsize", 20))),
	backgroundColor(parent?gColor(-1):gColor(0x20)),
	foregroundColor(parent?parent->foregroundColor:gColor(0x2F)), focus(0)
{
	LCDTitle=0;
	LCDElement=0;
	LCDTmp=0;
	target=parent?0:gFBDC::getInstance();
	in_loop=0;
	state=parent?stateShow:0;
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
}

void eWidget::takeFocus()
{
	if ((!parent) && !have_focus)
	{
		oldTLfocus=eZap::getInstance()->focus;
		eZap::getInstance()->focus=this;
		addActionMap(&i_focusActions->map);
	}
	have_focus++;
}

void eWidget::releaseFocus()
{
	if ((!parent) && have_focus)
	{
	 	have_focus--;
		if (!have_focus)
		{
			removeActionMap(&i_focusActions->map);
			if (eZap::getInstance()->focus==this)	// if we don't have lost the focus, ...
				eZap::getInstance()->focus=oldTLfocus;	// give it back
		}
 	}
}

void eWidget::_willShow()
{
	if (takefocus)
		getTLW()->takeFocus();
	willShow();
}

void eWidget::_willHide()
{
	willHide();
	if (takefocus)
		getTLW()->releaseFocus();
}

void eWidget::willShow()
{
}

void eWidget::willHide()
{
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
	if (isVisible())
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

void eWidget::invalidate(eRect area)
{
	if (!isVisible())
		return;

	if (area.isNull())
		area=eRect(0, 0, size.width(), size.height());

	eWidget *w=this;

	while (((int)w->getBackgroundColor())==-1)
	{
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
	
	if (!in_loop)	// hatte wohl jemand.
	{
		eWidget *ich=this;
		while (ich->parent)
			ich=ich->parent;
		result=ich->result;
	} else
	{
		in_loop=1;		// wir betreten die mainloop
		if (parent)		// ... allerdings die vom parent
			result=parent->exec();	// ... der uns auch das result gibt.
		else
			eApp->enter_loop();		// oder wir machen das halt selber.
		in_loop=0; // nu sind wir jedenfalls draussen.
	}
	event(eWidgetEvent(eWidgetEvent::execDone));
	
	return result;
}

void eWidget::clear()
{
	if (parent)
	{
		eRect me(getTLWPosition(), size);
		getTLW()->redraw(me);
	} else
	{
		gPainter *p=getPainter();
		p->setBackgroundColor(gColor(0));
		p->clear();
		delete p;
	}
}

void eWidget::close(int res)
{
	if (parent)
	{
		if (in_loop==1)
			parent->close(res);
		in_loop=0;	// in einer loop sind wir nun nichtmehr
	} else
	{
		if (in_loop==0)
			eFatal("attempt to close non-execing widget");
		if (in_loop==1)	// nur wenn das ne echte loop ist
			eApp->exit_loop();
		result=res;
	}
}

void eWidget::show()
{
	int oldstate=state;
	getTLW()->just_showing++;
	state|=stateShow;
	willShowChildren();

	checkFocus();
	getTLW()->just_showing--;
	if (!(oldstate&stateShow))
		redraw();
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
	if (state&stateShow)
	{
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
}

void eWidget::hide()
{
	if (state&stateShow)
	{
		willHideChildren();
		state&=~stateShow;
		clear();	// hide -> immer erasen. dieses Hide ist IMMER explizit.
		checkFocus();
	}
}

void eWidget::willHideChildren()
{
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
	if (!(getTLW()->focus && getTLW()->focus->isVisible()))
	{
		l->first();

		while (l->current() && !(l->current()->isVisible()))
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
	if (parent)
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
			if (_focusList.current() && _focusList.current()->isVisible())
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
			if (!i->isVisible())
				continue;
			ePoint m1=i->getPosition();
			m1+=ePoint(i->getSize().width()/2, i->getSize().height()/2);
			ePoint m2=_focusList.current()->getPosition();
			m2+=ePoint(_focusList.current()->getSize().width()/2, _focusList.current()->getSize().height()/2);
			
			int xd=m1.x()-m2.x();
			int yd=m1.y()-m2.y();
			int dif=xd*xd+yd*yd;
			int eff=0;
#if 0
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
	if (parent)
		return getTLW()->setFocus(newfocus);
	if (focus == newfocus)
		return;
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::lostFocus));
	focus=newfocus;
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::gotFocus));
	_focusList.setCurrent(focus);
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
	gPainter *p=new gPainter(*getTLW()->target, myclip);
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
	{
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
	}
	else if (prop=="font")
	{
		eString family=value;
		int sem=value.rfind(';');
		int size=16;
		if (sem!=-1)
		{
			family=family.left(sem);
			eString r=value.mid(sem+1);
			size=atoi(r.c_str());
			if (size<=0)
				size=16;
		}
		setFont(gFont(family, size));
	}
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

eAutoInitP0<eWidgetSkinInit> init_eWidgetSkinInit(3, "eWidget");
