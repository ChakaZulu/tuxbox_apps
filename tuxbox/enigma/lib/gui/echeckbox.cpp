#include "echeckbox.h"

#include <core/gdi/font.h>
#include <core/system/init.h>
#include <core/gui/eskin.h>

eCheckbox::eCheckbox(eWidget *parent, int checked, int takefocus, int Size):
	eButton(parent, 0, takefocus)
{
	ischecked = -1;
	setCheck(checked);
	CONNECT(selected, eCheckbox::sel);
}

eCheckbox::~eCheckbox()
{
}

void eCheckbox::sel()
{
	setCheck(ischecked^1);
	/*emit*/ checked(ischecked);
}

void eCheckbox::gotFocus()
{
	if (parent && parent->LCDElement)
	{
			LCDTmp = new eLabel(parent->LCDElement);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->move(ePoint(0,0));
			LCDTmp->resize(eSize(s.width(), s.height()));
			((eLabel*)LCDTmp)->setFlags(RS_WRAP);
			gPixmap *pm=eSkin::getActive()->queryImage(ischecked?"eCheckboxLCD.checked":"eCheckboxLCD.unchecked");
			LCDTmp->setPixmap(pm);
			((eLabel*)LCDTmp)->pixmap_position=ePoint(0, (size.height()-15)/2);
			((eLabel*)LCDTmp)->text_position=ePoint(21, 0);
			LCDTmp->setText(text);
			LCDTmp->show();
	}
	setBackgroundColor(focus);
	invalidate();
}

void eCheckbox::setCheck(int c)
{
	if (ischecked != -1 && ischecked == c)
		return;

	ischecked=c;
	gPixmap *pm=eSkin::getActive()->queryImage(ischecked?"eCheckbox.checked":"eCheckbox.unchecked");
	setPixmap(pm);

	if (LCDTmp)
	{
			gPixmap *pm=eSkin::getActive()->queryImage(ischecked?"eCheckboxLCD.checked":"eCheckboxLCD.unchecked");
			LCDTmp->setPixmap(pm);
	}
}

int eCheckbox::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		pixmap_position=ePoint(2, (size.height()-20)/2);
		text_position=ePoint(26, 0);
		break;
	default:
		break;
	}
	return 0;
}

static eWidget *create_eCheckbox(eWidget *parent)
{
	return new eCheckbox(parent);
}

class eCheckboxSkinInit
{
public:
	eCheckboxSkinInit()
	{
		eSkin::addWidgetCreator("eCheckbox", create_eCheckbox);
	}
	~eCheckboxSkinInit()
	{
		eSkin::removeWidgetCreator("eCheckbox", create_eCheckbox);
	}
};

eAutoInitP0<eCheckboxSkinInit> init_eCheckboxSkinInit(3, "eCheckbox");
