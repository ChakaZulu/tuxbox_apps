#include "ebutton.h"
#include "eskin.h"
#include "rc.h"

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
	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");

	setBackgroundColor(normal);
	redraw();
}

int eButton::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedText:
		break;
	}		
	eLabel::eventFilter(event);
}