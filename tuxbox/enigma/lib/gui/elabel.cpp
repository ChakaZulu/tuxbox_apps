#include "elabel.h"
#include "fb.h"
#include "font.h"
#include "lcd.h"
#include "eskin.h"
#include "init.h"
#include <qrect.h>

eLabel::eLabel(eWidget *parent, int flags, int takefocus):
	eWidget(parent, takefocus), flags(flags)
{
	para=0;
	align=eTextPara::dirLeft;
//	setBackgroundColor(eSkin::getActive()->queryScheme("fgColor"));
}

eLabel::~eLabel()
{
	invalidate();
}

void eLabel::invalidate()
{
	if (para)
		para->destroy();
	para=0;
}

void eLabel::validate()
{
	if (!para)
	{
		para=new eTextPara(QRect(0, 0, size.width(), size.height()));
		para->setFont(font);
		para->renderString(text, flags);
		para->realign(align);
	}
}

void eLabel::willHide()
{
	invalidate(); // ob das sinn macht müsste mal überlegt werden
}

void eLabel::setFlags(int flag)
{
	flags|=flag;
	if (flag)
		invalidate();
}

void eLabel::redrawWidget(gPainter *target, const QRect &area)
{
	if (isVisible())
	{
		validate();
		target->setFont(font);
		target->renderPara(*para);
	}
}

int eLabel::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedFont:
		invalidate();
		break;
	case eWidgetEvent::changedText:
		invalidate();
		break;
	}
	return 0;
}

QSize eLabel::getExtend()
{
	validate();
	return para->getExtend();
}

int eLabel::setProperty(const QString &prop, const QString &value)
{
	if (prop=="wrap")
		setFlags(RS_WRAP);
	else if (prop=="align")
	{
		if (value=="left")
			align=eTextPara::dirLeft;
		else if (value=="center")
			align=eTextPara::dirCenter;
		else if (value=="right")
			align=eTextPara::dirRight;
		else if (value=="block")
			align=eTextPara::dirBlock;
		else
			align=eTextPara::dirLeft;
	} else
		return eWidget::setProperty(prop, value);
	return 0;
}

static eWidget *create_eLabel(eWidget *parent)
{
	return new eLabel(parent);
}

class eLabelSkinInit
{
public:
	eLabelSkinInit()
	{
		eSkin::addWidgetCreator("eLabel", create_eLabel);
	}
	~eLabelSkinInit()
	{
		eSkin::removeWidgetCreator("eLabel", create_eLabel);
	}
};

eAutoInitP0<eLabelSkinInit> init_eLabelSkinInit(3, "eLabel");
