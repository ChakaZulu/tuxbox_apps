#include "eprogress.h"
#include "fb.h"
#include <erect.h>
#include <stdlib.h>
#include "lcd.h"
#include "eskin.h"
#include "init.h"

eProgress::eProgress(eWidget *parent)
	: eWidget(parent)
{
	left=eSkin::getActive()->queryScheme("eProgress.left");
	right=eSkin::getActive()->queryScheme("eProgress.right");
	perc=0;
	border=2;
	setForegroundColor(eSkin::getActive()->queryScheme("eProgress.border"));
}

eProgress::~eProgress()
{
}

void eProgress::setPerc(int p)
{
	perc=p;
	redraw();
}

void eProgress::redrawWidget(gPainter *target, const eRect &area)
{
	int dh=perc*(size.width()-border*2)/100;
	if (dh<0)
		dh=0;
	if (dh>(size.width()-border*2))
		dh=size.width()-border*2;
	target->setForegroundColor(getForegroundColor());
	target->fill(eRect(0, 0, size.width(), border));
	target->fill(eRect(0, border, border, size.height()-border));
	target->fill(eRect(border, size.height()-border, size.width()-border, border));
	target->fill(eRect(size.width()-border, border, border, size.height()-border));
	target->setForegroundColor(left);
	target->fill(eRect(border, border, dh, size.height()-border*2));
	target->setForegroundColor(right);
	target->fill(eRect(border+dh, border, size.width()-border*2-dh, size.height()-border*2));
}

int eProgress::setProperty(const eString &prop, const eString &value)
{
	if (prop=="leftColor")
		left=eSkin::getActive()->queryColor(value);
	else if (prop=="rightColor")
		right=eSkin::getActive()->queryColor(value);
	else if (prop=="border")
		border=atoi(value.c_str());
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

static eWidget *create_eProgress(eWidget *parent)
{
	return new eProgress(parent);
}

class eProgressSkinInit
{
public:
	eProgressSkinInit()
	{
		eSkin::addWidgetCreator("eProgress", create_eProgress);
	}
	~eProgressSkinInit()
	{
		eSkin::removeWidgetCreator("eProgress", create_eProgress);
	}
};

eAutoInitP0<eProgressSkinInit> init_eProgressSkinInit(3, "eProgress");
