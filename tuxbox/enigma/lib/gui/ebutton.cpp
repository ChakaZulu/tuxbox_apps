#include "ebutton.h"
#include "eskin.h"
#include <core/system/init.h>
#include <core/gdi/font.h>
#include "guiactions.h"

eButton::eButton(eWidget *parent, eLabel* desc, int takefocus, bool loadDeco)
	:eLabel(parent, 0, takefocus), tmpDescr(0), 
	focusB(eSkin::getActive()->queryScheme("global.selected.background")),
	focusF(eSkin::getActive()->queryScheme("global.selected.foreground")),
	normalB(eSkin::getActive()->queryScheme("global.normal.background")),
	normalF(eSkin::getActive()->queryScheme("global.normal.foreground")),
	descr(desc?desc->getText():"")
{
	if (!deco && loadDeco)
	{
		deco.load("eButton");
		deco_selected.load("eButton.selected");
	}

	if (deco)
	{
		align=eTextPara::dirCenter;
		setFlags( flagVCenter );
	}

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

				if (/*emit*/ selected_id(text) )  // description changed ?
					invalidate();										// then redraw
		
				if (parent && parent->LCDElement)
				{
					if (LCDTmp)
						LCDTmp->setText(text);
					else
						parent->LCDElement->setText(text);
				}
				return 1;
			}
		break;

		case eWidgetEvent::changedSize:
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
		break;

		default:
		break;
	}
	return eLabel::eventHandler(event);
}

void eButton::redrawWidget(gPainter *target, const eRect &area)
{
	if (deco_selected && have_focus)
	{
		deco_selected.drawDecoration(target, ePoint(width(), height()) );
		eLabel::redrawWidget(target, crect_selected);
	}
	else if (deco)
	{
		deco.drawDecoration(target, ePoint(width(), height()) );
		eLabel::redrawWidget(target, crect);
	}
	else
		eLabel::redrawWidget(target, area);

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
