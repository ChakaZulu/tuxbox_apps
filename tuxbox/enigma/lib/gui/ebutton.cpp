#include "ebutton.h"
#include "eskin.h"
#include <core/system/init.h>
#include "guiactions.h"

eButton::eButton(eWidget *parent, eLabel* desc, int takefocus)
	:eLabel(parent, 0, takefocus), tmpDescr(0), 
	focusB(eSkin::getActive()->queryScheme("global.selected.background")),
	focusF(eSkin::getActive()->queryScheme("global.selected.foreground")),
	normalB(eSkin::getActive()->queryScheme("global.normal.background")),
	normalF(eSkin::getActive()->queryScheme("global.normal.foreground")),
	descr(desc?desc->getText():"")
{
	addActionMap(&i_cursorActions->map);
}

void eButton::gotFocus()
{
	if (parent && parent->LCDElement)
	{
		if (descr != "")
		{
			LCDTmp = new eLabel(parent->LCDElement);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->move(ePoint(0,s.height()/2));
			LCDTmp->resize(eSize(s.width(), s.height()/2));
			LCDTmp->setText(text);
			LCDTmp->setBackgroundColor(255);
			LCDTmp->show();
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->hide();
			tmpDescr->move(ePoint(0,0));
			tmpDescr->resize(eSize(s.width(), s.height()/2));
			tmpDescr->setText(descr);
			tmpDescr->show();
		}
		else
			parent->LCDElement->setText(text);
	}

	setBackgroundColor(focusB);
	setForegroundColor(focusF);
	invalidate();
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
	setBackgroundColor(normalB);
	setForegroundColor(normalF);
	invalidate();
}

int eButton::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->ok)
		{
			/*emit*/ selected();
		
			if (parent && parent->LCDElement)
			{
				eString txt(text=="\x19"?"[X]":text=="\x18"?"[  ]":text);
				if (LCDTmp)
					LCDTmp->setText(txt);
				else
					parent->LCDElement->setText(txt);
			}
			return 1;
		}
		break;
	default:
		break;
	}
	return eWidget::eventHandler(event);
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

eAutoInitP0<eButtonSkinInit> init_eButtonSkinInit(3, "eButton");
