#include "ebutton.h"
#include "eskin.h"
#include "rc.h"
#include "init.h"

eButton::eButton(eWidget *parent, eLabel* descr, int takefocus):
	eLabel(parent, 0, takefocus), descr(descr?descr->getText():""), tmpDescr(0)
{
	focus=eSkin::getActive()->queryScheme("focusedColor");
	normal=eSkin::getActive()->queryScheme("fgColor");
}

void eButton::keyUp(int key)
{
	switch (key)
	{
	case eRCInput::RC_OK:
		emit selected();
		
		if (parent && parent->LCDElement)
		{
			if (LCDTmp)
				LCDTmp->setText(text);
			else
				parent->LCDElement->setText(text);
		}
	}
}

void eButton::gotFocus()
{
	if (parent && parent->LCDElement)
	{
		if (descr != "")
		{
			LCDTmp = new eLabel(parent->LCDElement);
			LCDTmp->hide();
			QSize s = parent->LCDElement->getSize();
			LCDTmp->move(QPoint(0,s.height()/2));
			LCDTmp->resize(QSize(s.width(), s.height()/2));
			LCDTmp->setText(text);
			LCDTmp->setBackgroundColor(255);
			LCDTmp->show();
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->hide();
			tmpDescr->move(QPoint(0,0));
			tmpDescr->resize(QSize(s.width(), s.height()/2));
			tmpDescr->setText(descr);
			tmpDescr->show();
		}
		else
				parent->LCDElement->setText(text);
	}

	setBackgroundColor(focus);
	redraw();
}

void eButton::lostFocus()
{
	if (parent && parent->LCDElement)
	{
		if (LCDTmp)
		{
			delete LCDTmp;
			LCDTmp = 0;
			if (tmpDescr)
			{
				delete tmpDescr;
				tmpDescr=0;
			}
		}
		else
			parent->LCDElement->setText("");	
	}
	setBackgroundColor(normal);
	redraw();
}

static eWidget *create_eButton(eWidget *parent)
{
	return new eButton(parent);
}

class eButtonSkinInit
{
public:
	eButtonSkinInit()
	{
		eSkin::addWidgetCreator("eButton", create_eButton);
	}
	~eButtonSkinInit()
	{
		eSkin::removeWidgetCreator("eButton", create_eButton);
	}
};

eAutoInitP0<eButtonSkinInit,3> init_eButtonSkinInit("eButton");
