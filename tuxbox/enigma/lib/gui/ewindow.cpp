#include "ewindow.h"
#include <core/gdi/grc.h>
#include <core/gui/eskin.h>
#include <core/system/init.h>
#include <core/gdi/epng.h>
#include <core/gui/elabel.h>
#include <core/gui/guiactions.h>

eWindow::eWindow(int takefocus)
	:eWidget(0, takefocus)
{
	deco.load("eWindow");

	titleBarColor=eSkin::getActive()->queryScheme("eWindow.titleBar");
	fontColor=eSkin::getActive()->queryScheme("eWindow.titleBarFont");

	borderLeft=eSkin::getActive()->queryValue("eWindow.borderLeft", deco.borderLeft);
	borderRight=eSkin::getActive()->queryValue("eWindow.borderRight", deco.borderRight);
	borderBottom=eSkin::getActive()->queryValue("eWindow.borderBottom", deco.borderBottom);
	borderTop=eSkin::getActive()->queryValue("eWindow.borderTop", deco.borderTop );

	titleOffsetX=eSkin::getActive()->queryValue("eWindow.titleOffsetX", 0);
	titleOffsetY=eSkin::getActive()->queryValue("eWindow.titleOffsetY", 0);
	titleHeight=eSkin::getActive()->queryValue("eWindow.titleHeight", titleFontSize+10);
	titleBorderY=eSkin::getActive()->queryValue("eWindow.titleBorderY", 0);
	titleFontSize=eSkin::getActive()->queryValue("eWindow.titleFontSize", 20);

	font = eSkin::getActive()->queryFont("eWindow.Childs");

	addActionMap(&i_cursorActions->map);
}

eWindow::~eWindow()
{
}

eRect eWindow::getTitleBarRect()
{
	return eRect(titleOffsetX, titleOffsetY, width()-titleOffsetX/*-titleBorderY*/, titleHeight);
}

void eWindow::redrawWidget(gPainter *target, const eRect &where)
{
	if ( deco )  // then draw Deco
		deco.drawDecoration(target, ePoint(width(), height()));

	if ( where.contains( getTitleBarRect() ) );
		drawTitlebar(target);

	if (LCDTitle)
		LCDTitle->setText(text);
}

void eWindow::eraseBackground(gPainter *target, const eRect &clip)
{
	target->clip(getClientRect());
	target->clear();
	target->flush();
	target->clippop();
}

void eWindow::drawTitlebar(gPainter *target)
{
	target->setForegroundColor(titleBarColor);
	target->fill( getTitleBarRect() );
	target->flush();

	target->setBackgroundColor(titleBarColor);
	target->setForegroundColor(fontColor);
	target->setFont( eSkin::getActive()->queryFont("eWindow.TitleBar") );
	target->renderText(getTitleBarRect(), text);
	target->flush();
}

void eWindow::recalcClientRect()
{
	clientrect=eRect(borderLeft, (titleOffsetY?titleOffsetY:borderTop)+titleHeight, size.width()-borderLeft-borderRight, size.height()-borderBottom-titleHeight-(titleOffsetY?titleOffsetY:borderTop));
}

int eWindow::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedText:
			redraw( getTitleBarRect() );
		return 1;

		case eWidgetEvent::evtAction:
			if ((event.action == &i_cursorActions->cancel) && in_loop)	// hack
			{
				close(-1);
				return eWidget::eventHandler(event);
			}
			else
				break;
			return 1;
		default:
			break;
	}
	return eWidget::eventHandler(event);
}


void eWindow::willShow()
{
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
