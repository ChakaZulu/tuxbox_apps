#include "elabel.h"

#include <core/gdi/fb.h>
#include <core/gdi/font.h>
#include <core/gdi/lcd.h>
#include <core/gui/eskin.h>
#include <core/system/init.h>

eLabel::eLabel(eWidget *parent, int flags, int takefocus ):
	eWidget(parent, takefocus), blitFlags(0), flags(flags), para(0), align(eTextPara::dirLeft)
{
	setForegroundColor(eSkin::getActive()->queryScheme("global.normal.foreground"));
}

eLabel::~eLabel()
{
	invalidate();
}

void eLabel::validate( const eSize* s )
{
	if (!para)
  {
		if (s)
			para=new eTextPara(eRect(text_position.x(), text_position.y(), s->width() - text_position.x(), s->height() - text_position.y()));		
		else
			para=new eTextPara(eRect(text_position.x(), text_position.y(), size.width() - text_position.x(), size.height() - text_position.y()));
		para->setFont(font);
		para->renderString(text, flags);
		para->realign(align);
  }
}

void eLabel::invalidate()
{
	if (para)
		para->destroy();
	para=0;
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

void eLabel::removeFlags(int flag)
{
	flags &= ~flag;
	if (flag)
		invalidate();
}

void eLabel::setAlign(int align)
{
	this->align = align;
	invalidate();
}

void eLabel::redrawWidget(gPainter *target, const eRect &area)
{
	if (text.length())
	{
		if ( area.size().height() < size.height() || area.size().width() < size.width() )
		{
			invalidate();
			validate( &area.size() );
		}
		else
			validate();

		if (flags & flagVCenter)
			yOffs = ( (area.height() - para->getBoundBox().height() ) / 2 + 0) - para->getBoundBox().top();
		else
			yOffs = 0;

		eWidget *w=getNonTransparentBackground();
		target->setBackgroundColor(w->getBackgroundColor());
		target->setFont(font);
		target->renderPara(*para, ePoint( area.left(), area.top()+yOffs) );
	}
	if (pixmap)
		target->blit(*pixmap, pixmap_position, eRect(), (blitFlags & BF_ALPHATEST) ? gPixmap::blitAlphaTest : 0);
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
	default:
		break;
	}
	return 0;
}

eSize eLabel::getExtend()
{
	validate();
	return eSize(para->getBoundBox().width(), para->getBoundBox().height());
}

ePoint eLabel::getLeftTop()
{
	validate();
	return ePoint(para->getBoundBox().left(), para->getBoundBox().top());
}

int eLabel::setProperty(const eString &prop, const eString &value)
{
	if (prop=="wrap" && value == "on")
		setFlags(RS_WRAP);
	else if (prop=="alphatest" && value == "on")
		blitFlags |= BF_ALPHATEST;
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
	}
	else if (prop=="vcenter")
		setFlags( flagVCenter );
	else
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
