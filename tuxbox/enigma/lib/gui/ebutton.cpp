#include "ebutton.h"
#include "eskin.h"
#include "rc.h"

eButton::eButton(eWidget *parent):
	eLabel(parent, 0, 1)
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
	}
}

void eButton::gotFocus()
{
	setBackgroundColor(focus);
	redraw();
}

void eButton::lostFocus()
{
	setBackgroundColor(normal);
	redraw();
}
