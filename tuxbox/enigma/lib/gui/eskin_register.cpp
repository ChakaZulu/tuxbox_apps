#include "eskin_register.h"
#include "eskin.h"

#include "ewindow.h"
#include "elabel.h"
#include "echeckbox.h"
#include "ebutton.h"
#include "eprogress.h"
#include "epixmap.h"

eWidget *create_eWindow(eWidget *parent)
{
	return new eWindow();
}

eWidget *create_eLabel(eWidget *parent)
{
	return new eLabel(parent);
}

eWidget *create_eCheckbox(eWidget *parent)
{
	return new eCheckbox(parent);
}

eWidget *create_eButton(eWidget *parent)
{
	return new eButton(parent);
}

eWidget *create_eProgress(eWidget *parent)
{
	return new eProgress(parent);
}

eWidget *create_ePixmap(eWidget *parent)
{
	return new ePixmap(parent);
}

void eSkin_init()
{
	eSkin::addWidgetCreator("eWindow", create_eWindow);
	eSkin::addWidgetCreator("eLabel", create_eLabel);
	eSkin::addWidgetCreator("eCheckbox", create_eCheckbox);
	eSkin::addWidgetCreator("eButton", create_eButton);
	eSkin::addWidgetCreator("eProgress", create_eProgress);
	eSkin::addWidgetCreator("ePixmap", create_ePixmap);
}

void eSkin_close()
{
}
