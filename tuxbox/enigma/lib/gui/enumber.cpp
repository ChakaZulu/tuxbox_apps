#include <lib/gui/enumber.h>
#include <lib/driver/rc.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gdi/fb.h>
#include <lib/gdi/grc.h>
#include <lib/gui/guiactions.h>

eRect eNumber::getNumberRect(int n)
{
	if (deco_selected && have_focus)
		return eRect( deco_selected.borderLeft + n * space_selected, deco_selected.borderTop, dspace, crect_selected.height() );
	else if (deco)
		return eRect( deco.borderLeft + n * dspace, deco.borderTop, dspace, crect.height() );
	else
		return eRect( n * dspace, 0, dspace, height() );
}

void eNumber::redrawNumber(gPainter *p, int n, const eRect &area)
{
	eRect pos =	getNumberRect(n);

	if (!area.contains(pos) )
		return;

	p->setForegroundColor((have_focus && n==active)?cursorB:normalB);
	p->fill(pos);
	p->setFont(font);

	eString t;
	if (flags & flagFillWithZeros)
	{
    eString s = "%0"+eString().setNum(maxdigits)+(base==10?"d":"X");
		const char* p = s.c_str();
		char* tmp = new char[10];
		strcpy( tmp, p );
		t.sprintf(tmp, number[n]);
		delete tmp;
	}
	else
	{
		if (flags&flagHideInput)
			t="*";
		else if (base==10)
			t.sprintf("%d", number[n]);
		else if (base==0x10)
			t.sprintf("%X", number[n]);
	}

	if (!n && flags & flagPosNeg && neg)
		t="-"+t;

	if (n && (flags & flagTime))
		t=":"+t;

	else if (n && (flags & flagDrawPoints))
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

	if (deco_selected && have_focus)
		deco_selected.drawDecoration(p, ePoint(width(), height()));
	else if (deco)
		deco.drawDecoration(p, ePoint(width(), height()));
}

void eNumber::invalidateNum()
{
	if ( have_focus && deco_selected )
		invalidate( crect_selected );
	else if ( deco )
		invalidate( crect );
	else
		invalidate();
}

int eNumber::eventHandler(const eWidgetEvent &event)
{
	if (LCDTmp)
		((eNumber*) LCDTmp)->eventHandler(event);

	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		if (deco)
			dspace = (crect.width()) / len;
		else
			dspace = (size.width()) / len;	
		if (deco_selected)
			space_selected = (crect_selected.width()) / len;
		break;
	case eWidgetEvent::evtAction:
		if ( len > 1 && event.action == &i_cursorActions->left)
		{
			int oldac=active;
			active--;
			invalidate(getNumberRect(oldac));
			if (active<0)
				active=len-1;
			if (active!=oldac)
				invalidate(getNumberRect(active));
			digit=0;
		} else if ( len > 1 && (event.action == &i_cursorActions->right) || (event.action == &i_cursorActions->ok))
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
	return eDecoWidget::eventHandler(event);
}

eNumber::eNumber(eWidget *parent, int _len, int _min, int _max, int _maxdigits, int *init, int isactive, eWidget* descr, int grabfocus, const char *deco)
 :eDecoWidget(parent, grabfocus, deco ),
	active(0), 
	cursorB(eSkin::getActive()->queryScheme("global.selected.background")),	
	cursorF(eSkin::getActive()->queryScheme("global.selected.foreground")),	
	normalB(eSkin::getActive()->queryScheme("global.normal.background")),	
	normalF(eSkin::getActive()->queryScheme("global.normal.foreground")),	
	have_focus(0), digit(isactive), isactive(isactive), descr(descr), tmpDescr(0),
  neg(false)
{
	setNumberOfFields(_len);
	setLimits(_min, _max);
	setMaximumDigits(_maxdigits);
	setFlags(0);
	setBase(10);
	for (int i=0; init && i<len; i++)
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
		if (flags & flagTime)
			if ( active == 0 )
				min=0, max = 23;
			else
				min=0, max = 59;
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

	break;
	}
	case eRCInput::RC_PLUS:
		if (flags & flagPosNeg && neg )
		{
			neg=false;
			invalidate(getNumberRect(0));
		}
	break;

	case eRCInput::RC_MINUS:
		if (flags & flagPosNeg && !neg )
		{
			neg=true;
			invalidate(getNumberRect(0));
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
	
  if (deco && deco_selected)
		invalidate();
	else
		invalidate(getNumberRect(active));

	if (parent && parent->LCDElement)  // detect if LCD Avail
	{
		LCDTmp = new eNumber(parent->LCDElement, len, min, max, maxdigits, &(number[0]), isactive, 0, 0);
		LCDTmp->hide();
		((eNumber*)LCDTmp)->setFlags(flags);
		eSize s = parent->LCDElement->getSize();

		if (descr)
		{
			LCDTmp->move(ePoint(0,s.height()/2));
			LCDTmp->resize(eSize(s.width(), s.height()/2));
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->hide();
			tmpDescr->move(ePoint(0,0));
			tmpDescr->resize(eSize(s.width(), s.height()/2));
			tmpDescr->setText(descr->getText());
			tmpDescr->show();
		}
		else
		{
			LCDTmp->resize(s);
			LCDTmp->move(ePoint(0,0));
		}
		((eNumber*)LCDTmp)->digit=digit;
		((eNumber*)LCDTmp)->active=active;
		((eNumber*)LCDTmp)->normalF=255;
		((eNumber*)LCDTmp)->normalB=0;
		((eNumber*)LCDTmp)->cursorF=0;
		((eNumber*)LCDTmp)->cursorB=255;
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

	if (deco && deco_selected)
		invalidate();
	else
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
  if (len == 1)
  {
		if ( flags&flagPosNeg && n < 0 )
		{
			neg=1;
			n = abs(n);
		}
		number[0]=n;
	}
	else
		for (int i=len-1; i>=0; --i)
		{
			number[i]=n%base;
			n/=base;
		}

	invalidateNum();
}

int eNumber::getNumber()
{
	int n=0;
	for (int i=0; i<len; i++)
	{
		n*=base;
		n+=number[i];
	}
	return flags&flagPosNeg && neg ? -n : n;
}
