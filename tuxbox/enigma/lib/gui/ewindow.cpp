#include "ewindow.h"
#include "enigma.h"
#include "grc.h"
#include "eskin.h"
#include "init.h"
#include "epng.h"
#include "elabel.h"

eWindow::eWindow(int takefocus)
	:eWidget(0, takefocus)
{
	titleSize=10;
	border=5;
	setBackgroundColor(eSkin::getActive()->queryScheme("backgroundColor"));
	setForegroundColor(eSkin::getActive()->queryScheme("eWindow.titleBarColor"));

	iTopLeft=eSkin::getActive()->queryImage("eWindow.topLeft");
	iTop=eSkin::getActive()->queryImage("eWindow.top");
	iTopRight=eSkin::getActive()->queryImage("eWindow.topRight");
	iLeft=eSkin::getActive()->queryImage("eWindow.left");
	iRight=eSkin::getActive()->queryImage("eWindow.right");
	iBottomLeft=eSkin::getActive()->queryImage("eWindow.bottomLeft");
	iBottom=eSkin::getActive()->queryImage("eWindow.bottom");
	iBottomRight=eSkin::getActive()->queryImage("eWindow.bottomRight");
	
	if (iLeft)
		if (border<iLeft->x)
			border=iLeft->x;
	if (iRight)
		if (border<iRight->x)
			border=iRight->x;
	if (iTopLeft)
		titleSize=iTopLeft->y;
	
	border=eSkin::getActive()->queryValue("eWindow.borderSize", border);
	titleSize=eSkin::getActive()->queryValue("eWindow.titleSize", 30);
	titleOffsetX=eSkin::getActive()->queryValue("eWindow.titleOffsetX", 10);
	titleOffsetY=eSkin::getActive()->queryValue("eWindow.titleOffsetY", 10);
	titleFontSize=eSkin::getActive()->queryValue("eWindow.titleFontSize", 20);
}

eWindow::~eWindow()
{
}

void eWindow::OnFontSizeChanged(int NewFontSize)
{
}

void eWindow::redrawWidget(gPainter *target, const eRect &where)
{
	if (where.contains(eRect(0, 0, width(), titleSize)))
		drawTitlebar(target);

	if (LCDTitle)
		LCDTitle->setText(text);
}

void eWindow::drawTitlebar(gPainter *target)
{
	int x=0, xm=width(), y, ym;
	
	if (iTopLeft)
	{
		target->blit(*iTopLeft, ePoint(0, 0));
		target->flush();
		x+=iTopLeft->x;
	}

	if (iTopRight)
	{
		xm-=iTopRight->x;
		target->blit(*iTopRight, ePoint(xm, 0), eRect(x, 0, width()-x, height()));
		target->flush();
	}
	
	if (iTop)
	{
		while (x<xm)
		{
			target->blit(*iTop, ePoint(x, 0), eRect(x, 0, xm-x, height()));
			x+=iTop->x;
		}
		target->flush();
	} else
	{
		target->fill(eRect(0, 0, width(), titleSize));
		target->flush();
	}

	x=0;
	xm=width();

	if (iBottomLeft)
	{
		target->blit(*iBottomLeft, ePoint(0, height()-iBottomLeft->y));
		target->flush();
		x+=iBottomLeft->x;
	}

	if (iBottomRight)
	{
		xm-=iBottomRight->x;
		target->blit(*iBottomRight, ePoint(xm, height()-iBottomRight->y), eRect(x, height()-iBottomRight->y, width()-x, iBottomRight->y));
		target->flush();
	}
	
	if (iBottom)
	{
		while (x<xm)
		{
			target->blit(*iBottom, ePoint(x, height()-iBottom->y), eRect(x, height()-iBottom->y, xm-x, iBottom->y));
			x+=iBottom->x;
		}
		target->flush();
	}
	
	y=0; ym=height();
	
	if (iTopLeft)
		y=iTopLeft->y;
	if (iBottomLeft)
		ym=height()-iBottomLeft->y;
	if (iLeft)
	{
		while (y<ym)
		{
			target->blit(*iLeft, ePoint(0, y), eRect(0, y, iLeft->x, ym-y));
			y+=iLeft->y;
		}
	}

	if (iTopRight)
		y=iTopRight->y;
	if (iBottomRight)
		ym=height()-iBottomRight->y;
	if (iRight)
	{
		while (y<ym)
		{
			target->blit(*iRight, ePoint(width()-iRight->x, y), eRect(width()-iRight->x, y, iRight->x, ym-y));
			y+=iRight->y;
		}
	}
	
	target->flush();

	target->setFont(gFont("NimbusSansL-Regular Sans L Regular", titleFontSize));
	target->renderText(eRect(titleOffsetX, titleOffsetY, width()-titleOffsetX, titleSize), text);
	target->flush();
}

void eWindow::recalcClientRect()
{
	clientrect=eRect(border, titleSize, size.width()-border*2, size.height()-titleSize-border);
}

int eWindow::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedText:
		redraw(eRect(0, 0, width(), titleSize));
		return 1;
		break;
	}
	return 0;
}

void eWindow::willShow()
{
/*	if (LCDTitle)
		LCDTitle->setText(text);*/
}

void eWindow::willHide()
{
}

static eWidget *create_eWindow(eWidget *parent)
{
	return new eWindow();
}

class eWindowSkinInit
{
public:
	eWindowSkinInit()
	{
		eSkin::addWidgetCreator("eWindow", create_eWindow);
	}
	~eWindowSkinInit()
	{
		eSkin::removeWidgetCreator("eWindow", create_eWindow);
	}
};

eAutoInitP0<eWindowSkinInit> init_eWindowSkinInit(3, "eWindow");
