#include "ebutton.h"
#include "eskin.h"
#include "rc.h"
#include "init.h"

eButton::eButton(eWidget *parent, eLabel* descr):
	eLabel(parent, 0, 1), descr(descr?descr->getText():"")
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
			parent->LCDElement->setText(descr!=""?descr+'\n'+text:text);
	}
}

void eButton::gotFocus()
{
	if (parent && parent->LCDElement)
		parent->LCDElement->setText(descr!=""?descr+'\n'+text:text);

	setBackgroundColor(focus);
	redraw();
}

void eButton::lostFocus()
{
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
