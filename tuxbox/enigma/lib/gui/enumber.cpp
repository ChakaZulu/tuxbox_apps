#include "enumber.h"
#include "fb.h"
#include "rc.h"
#include "eskin.h"

eRect eNumber::getNumberRect(int n)
{
	return eRect(n*space, 0, space, size.height());
}

void eNumber::redrawNumber(gPainter *p, int n, const eRect &area)
{
	eRect pos=eRect(n*space, 0, space, size.height());

	if (!area.contains(pos))
		return;
	
	p->setForegroundColor((have_focus && n==active)?cursor:normal);
	p->fill(pos);
	p->setFont(font);
	p->renderText(pos, eString().sprintf("%s%d", n?".":"", number[n]));
	p->flush();
}

void eNumber::redrawWidget(gPainter *p, const eRect &area)
{
	for (int i=0; i<len; i++)
		redrawNumber(p, i, area);
}

int eNumber::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		space=(size.width()-2)/len;
		break;
	}
	return 0;
}

eNumber::eNumber(eWidget *parent, int len, int min, int max, int maxdigits, int *init, int isactive, eLabel* descr, int grabfocus)
	:eWidget(parent, grabfocus), len(len), min(min), max(max), maxdigits(maxdigits), isactive(isactive), descr(descr?descr->getText():""),
	active(0), digit(isactive),have_focus(0), cursor(cursor=eSkin::getActive()->queryScheme("focusedColor")),	normal(eSkin::getActive()->queryScheme("fgColor")),
	tmpDescr(0)
{
	for (int i=0; i<len; i++)
		number[i]=init[i];
}

eNumber::~eNumber()
{
}

int eNumber::keyUp(int key)
{
	return 0;
}

int eNumber::keyDown(int key)
{
	if (LCDTmp)
		((eNumber*) LCDTmp)->keyDown(key);

	switch (key)
	{
	case eRCInput::RC_OK:
	case eRCInput::RC_RIGHT:
	{
		int oldac=active;
		active++;
		invalidate(getNumberRect(oldac));
		if (active>=len)
		{
			if (key==eRCInput::RC_OK)
				/*emit*/ selected(number);
			active=0;
		}

		if (active!=oldac)
			invalidate(getNumberRect(active));
		digit=0;
		break;
	}
	case eRCInput::RC_LEFT:
	{
		int oldac=active;
		active--;
		invalidate(getNumberRect(oldac));
		if (active<0)
			active=len-1;
		if (active!=oldac)
			invalidate(getNumberRect(active));
		digit=0;
		break;
	}
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
	if (parent && parent->LCDElement)
	{
		if (descr != "")
		{
			LCDTmp = new eNumber(parent->LCDElement, len, min, max, maxdigits, &(number[0]), isactive, 0, 0);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->move(ePoint(0,s.height()/2));
			LCDTmp->resize(eSize(s.width(), s.height()/2));
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->move(ePoint(0,0));
			tmpDescr->resize(eSize(s.width(), s.height()/2));
			tmpDescr->setText(descr);
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
		((eNumber*)LCDTmp)->normal=0;
		((eNumber*)LCDTmp)->cursor=255;
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
