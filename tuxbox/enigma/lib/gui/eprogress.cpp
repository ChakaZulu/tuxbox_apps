#include <lib/gui/eprogress.h>

#include <stdlib.h>

#include <lib/base/erect.h>
#include <lib/gdi/fb.h>
#include <lib/gdi/lcd.h>
#include <lib/gui/eskin.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

eProgress::eProgress(eWidget *parent, int takefocus)
	: eWidget(parent, takefocus)
{
	left=eSkin::getActive()->queryScheme("eProgress.left");
	right=eSkin::getActive()->queryScheme("eProgress.right");
	perc=start=0;
	border=2;
	direction=0;
	setForegroundColor(eSkin::getActive()->queryScheme("eProgress.border"));
}

eProgress::~eProgress()
{
}

bool eProgress::setParams( int _start, int _perc )
{
	if ( perc != _perc || start != _start )
	{
		perc = _perc;
		start = _start;
		invalidate();
		return true;
	}
	return false;
}

void eProgress::setPerc(int p)
{
	if (perc != p)
	{
		perc=p;
		invalidate();
	}
}

void eProgress::setStart(int p)
{
	if (start != p)
	{
		start=p;
		invalidate();
	}
}

void eProgress::redrawWidget(gPainter *target, const eRect &area)
{
	// border malen
	target->setForegroundColor(getForegroundColor());
	target->fill(eRect(0, 0, size.width(), border));
	target->fill(eRect(0, border, border, size.height()-border));
	target->fill(eRect(border, size.height()-border, size.width()-border, border));
	target->fill(eRect(size.width()-border, border, border, size.height()-border));

	switch (direction)
	{
	case 0:
	{
		int st=start*(size.width()-border*2)/100;
		if (st<0)
			st=0;
		if (st>(size.width()-border*2))
			st=size.width()-border*2;

		int dh=perc*(size.width()-border*2)/100;
		if (dh<0)
			dh=0;
		if ((dh+st)>(size.width()-border*2))
			dh=size.width()-border*2-st;

		target->setForegroundColor(left);
		target->fill(eRect(border+start, border, dh, size.height()-border*2));
		target->setForegroundColor(right);
		target->fill(eRect(border+dh+st, border, size.width()-border*2-dh-st, size.height()-border*2));
		if (st)
			target->fill(eRect(border, border, st, size.height()-border*2));
		break;
	}
	case 1:
	{
		int st=start*(size.height()-border*2)/100;
		if (st<0)
			st=0;
		if (st>(size.height()-border*2))
			st=size.height()-border*2;

		int dh=perc*(size.height()-border*2)/100;
		if (dh<0)
			dh=0;
		if ((dh+st)>(size.height()-border*2))
			dh=size.height()-border*2-st;

		target->setForegroundColor(left);
		target->fill(eRect(border, border+st, size.width()-border*2, dh));
		target->setForegroundColor(right);
		target->fill(eRect(border, border+dh+st, size.width()-border*2, size.height()-border*2-dh-st));
		if (st)
			target->fill(eRect(border, border, size.width()-border*2, st));
		break;
	}
	}
}

int eProgress::setProperty(const eString &prop, const eString &value)
{
	if (prop=="leftColor")
		left=eSkin::getActive()->queryColor(value);
	else if (prop=="rightColor")
		right=eSkin::getActive()->queryColor(value);
	else if (prop=="border")
		border=atoi(value.c_str());
	else if (prop=="direction")
		direction=atoi(value.c_str());
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

eAutoInitP0<eProgressSkinInit> init_eProgressSkinInit(eAutoInitNumbers::guiobject, "eProgress");
