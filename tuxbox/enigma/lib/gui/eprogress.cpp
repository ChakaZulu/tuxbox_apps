#include "eprogress.h"
#include "fb.h"
#include <qrect.h>
#include "lcd.h"
#include "eskin.h"

eProgress::eProgress(eWidget *parent)
	: eWidget(parent)
{
	left=eSkin::getActive()->queryScheme("eProgress.left");
	right=eSkin::getActive()->queryScheme("eProgress.right");
	perc=0;
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

void eProgress::redrawWidget(gPainter *target, const QRect &area)
{
	int dh=perc*(size.width()-4)/100;
	if (dh<0)
		dh=0;
	if (dh>(size.width()-4))
		dh=size.width()-4;
	target->setForegroundColor(getForegroundColor());
	target->fill(QRect(0, 0, size.width(), 2));
	target->fill(QRect(0, 2, 2, size.height()-2));
	target->fill(QRect(2, size.height()-2, size.width()-2, 2));
	target->fill(QRect(size.width()-2, 2, 2, size.height()-2));
	target->setForegroundColor(left);
	target->fill(QRect(2, 2, dh, size.height()-4));
	target->setForegroundColor(right);
	target->fill(QRect(2+dh, 2, size.width()-4-dh, size.height()-4));
}

int eProgress::setProperty(const QString &prop, const QString &value)
{
	if (prop=="leftColor")
		left=eSkin::getActive()->queryColor(value);
	else if (prop=="rightColor")
		right=eSkin::getActive()->queryColor(value);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}
