#include <lib/gui/ebutton.h>
#include <lib/gui/eskin.h>
#include <lib/system/init.h>
#include <lib/gdi/font.h>
#include <lib/gui/guiactions.h>

eButton::eButton(eWidget *parent, eLabel* desc, int takefocus, const char *deco)
	:eLabel(parent, 0, takefocus, deco), tmpDescr(0),
	focusB(eSkin::getActive()->queryScheme("global.selected.background")),
	focusF(eSkin::getActive()->queryScheme("global.selected.foreground")),
	normalB(eSkin::getActive()->queryScheme("global.normal.background")),
	normalF(eSkin::getActive()->queryScheme("global.normal.foreground")),
	descr(desc)
{
	align=eTextPara::dirCenter;
	flags |= eLabel::flagVCenter;
	addActionMap(&i_cursorActions->map);
}

void eButton::gotFocus()
{
	if (parent && parent->LCDElement)
	{
		if (descr)
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
			tmpDescr->setText(descr->getText());
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
				/*emit*/ selected_id(this);
		
				if (parent && parent->LCDElement)
				{
					if (LCDTmp)
						LCDTmp->setText(text);
					else
						parent->LCDElement->setText(text);
				}
			}
			else
				return eLabel::eventHandler(event);
		break;

		default:
			return eLabel::eventHandler(event);
		break;
	}
	return 1;
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
