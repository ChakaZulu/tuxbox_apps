#include "enumber.h"
#include <core/driver/rc.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>
#include <core/gdi/fb.h>
#include <core/gdi/grc.h>
#include <core/gui/guiactions.h>

eRect eNumber::getNumberRect(int n)
{
	return eRect(n*space, 0, space, size.height());
}

void eNumber::redrawNumber(gPainter *p, int n, const eRect &area)
{
	eRect pos=eRect(n*space, 0, space, size.height());

	if (!area.contains(pos))
		return;
		
	p->setForegroundColor((have_focus && n==active)?cursorB:normalB);
	p->fill(pos);
	p->setFont(font);
	
	eString t;
	if (base==10)
		t.sprintf("%d", number[n]);
	else if (base==0x10)
		t.sprintf("%X", number[n]);

	if (n && (flags & flagDrawPoints))
		t="."+t;
	
	p->setForegroundColor((have_focus && n==active)?cursorF:normalF);
	p->setBackgroundColor((have_focus && n==active)?cursorB:normalB);
	p->renderText(pos, t);
	p->flush();
}

void eNumber::redrawWidget(gPainter *p, const eRect &area)
{
	for (int i=0; i<len; i++)
		redrawNumber(p, i, area);
}

int eNumber::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		space=(size.width()-2)/len;
		break;
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->left)
		{
			int oldac=active;
			active--;
			invalidate(getNumberRect(oldac));
			if (active<0)
				active=len-1;
			if (active!=oldac)
				invalidate(getNumberRect(active));
			digit=0;
		} else if ((event.action == &i_cursorActions->right) || (event.action == &i_cursorActions->ok))
		{
			int oldac=active;
			active++;
			invalidate(getNumberRect(oldac));
			if (active>=len)
			{
				if (event.action == &i_cursorActions->ok)
					/*emit*/ selected(number);
				active=0;
			}
	
			if (active!=oldac)
				invalidate(getNumberRect(active));
			digit=0;
		} else
			break;
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

eNumber::eNumber(eWidget *parent, int _len, int _min, int _max, int _maxdigits, int *init, int isactive, eLabel* descr, int grabfocus)
	:eWidget(parent, grabfocus), 
	active(0), 
	cursorB(eSkin::getActive()->queryScheme("global.selected.background")),	
	cursorF(eSkin::getActive()->queryScheme("global.selected.foreground")),	
	normalB(eSkin::getActive()->queryScheme("global.normal.background")),	
	normalF(eSkin::getActive()->queryScheme("global.normal.foreground")),	
	have_focus(0), digit(isactive), isactive(isactive), descr(descr?descr->getText():""), tmpDescr(0)
{
	setNumberOfFields(_len);
	setLimits(_min, _max);
	setMaximumDigits(_maxdigits);
	setFlags(0);
	setBase(10);
	for (int i=0; i<len; i++)
		number[i]=init[i];
	addActionMap(&i_cursorActions->map);
}

eNumber::~eNumber()
{
}

int eNumber::keyDown(int key)
{
	if (LCDTmp)
		((eNumber*) LCDTmp)->keyDown(key);

	switch (key)
	{
	case eRCInput::RC_0 ... eRCInput::RC_9:
	{
		int nn=(digit!=0)?number[active]*10:0;
		nn+=key-eRCInput::RC_0;
		if (nn>=min && nn<=max)
		{
			number[active]=nn;
			invalidate(getNumberRect(active));
			digit++;
			if ((digit>=maxdigits) || (nn==0))
			{
				active++;
				invalidate(getNumberRect(active-1));
				digit=0;
				/*emit*/ numberChanged();
				if (active>=len)
				{
					/*emit*/ selected(number);
					active=0;
				}
				else
					invalidate(getNumberRect(active));
			}
		}
		break;
	}
	default:
		return 0;
	}
	return 1;
}

void eNumber::gotFocus()
{
	have_focus++;
	digit=isactive;
	invalidate(getNumberRect(active));
	if (parent && parent->LCDElement)  // detect if LCD Avail
	{
		if (descr != "")
		{
			LCDTmp = new eNumber(parent->LCDElement, len, min, max, maxdigits, &(number[0]), isactive, 0, 0);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->move(ePoint(0,s.height()/2));
			LCDTmp->resize(eSize(s.width(), s.height()/2));
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->hide();
			tmpDescr->move(ePoint(0,0));
			tmpDescr->resize(eSize(s.width(), s.height()/2));
			tmpDescr->setText(descr);
			tmpDescr->show();
		}
		else
		{
			LCDTmp = new eNumber(parent->LCDElement, len, min, max, maxdigits, &(number[0]), isactive, 0, 0);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->resize(s);
			LCDTmp->move(ePoint(0,0));
		}
		((eNumber*)LCDTmp)->digit=digit;
		((eNumber*)LCDTmp)->active=active;
		((eNumber*)LCDTmp)->normalF=0;
		((eNumber*)LCDTmp)->cursorF=255;
		((eNumber*)LCDTmp)->normalB=255;
		((eNumber*)LCDTmp)->cursorB=0;
		((eNumber*)LCDTmp)->have_focus=1;
		LCDTmp->show();
	}
}

void eNumber::lostFocus()
{
	if (LCDTmp)
	{
		delete LCDTmp;
		LCDTmp=0;
		if (tmpDescr)
		{
			delete tmpDescr;
			tmpDescr=0;
		}
	}
	have_focus--;
	invalidate(getNumberRect(active));
}

void eNumber::setNumber(int f, int n)
{
	if ((f>=0) && (f<len))
		number[f]=n;
	invalidate(getNumberRect(f));
}

void eNumber::setLimits(int _min, int _max)
{
	min=_min;
	max=_max;
}

void eNumber::setNumberOfFields(int n)
{
	len=n;
}

void eNumber::setMaximumDigits(int n)
{
	if (n > 16)
		n=16;
	maxdigits=n;
	if (digit >= maxdigits)
		digit=0;
}

void eNumber::setFlags(int _flags)
{
	flags=_flags;
}

void eNumber::setBase(int _base)
{
	base=_base;
}

void eNumber::setNumber(int n)
{
	for (int i=len-1; i>=0; --i)
	{
		number[i]=n%base;
		n/=base;
	}
}

int eNumber::getNumber()
{
	int n=0;
	for (int i=0; i<len; i++)
	{
		n*=base;
		n+=number[i];
	}
	return n;
}
