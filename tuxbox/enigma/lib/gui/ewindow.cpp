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

	borderTop=deco.borderTop;
	borderLeft=deco.borderLeft;
	borderRight=deco.borderRight;
	borderBottom=deco.borderBottom;
	
// setBackgroundColor(eSkin::getActive()->queryScheme("global.normal.background"));
	titleBarColor=eSkin::getActive()->queryScheme("eWindow.titleBar");
	fontColor=eSkin::getActive()->queryScheme("eWindow.titleBarFont");

	borderLeft=eSkin::getActive()->queryValue("eWindow.borderLeft", borderLeft);
	borderRight=eSkin::getActive()->queryValue("eWindow.borderRight", borderRight);
	borderBottom=eSkin::getActive()->queryValue("eWindow.borderBottom", borderBottom);
	borderTop=eSkin::getActive()->queryValue("eWindow.borderTop", 30);
	titleOffsetX=eSkin::getActive()->queryValue("eWindow.titleOffsetX", 10);
	titleOffsetY=eSkin::getActive()->queryValue("eWindow.titleOffsetY", 10);
	titleBorderY=eSkin::getActive()->queryValue("eWindow.titleBorderY", 0);
	titleFontSize=eSkin::getActive()->queryValue("eWindow.titleFontSize", 20);
	titleHeight=eSkin::getActive()->queryValue("eWindow.titleHeight", titleFontSize+10);

	font = eSkin::getActive()->queryFont("eWindow.Childs");

	addActionMap(&i_cursorActions->map);
}

eWindow::~eWindow()
{
}

void eWindow::redrawWidget(gPainter *target, const eRect &where)
{
	if (where.contains(eRect(0, 0, width(), borderTop)))
		drawTitlebar(target);

	if (LCDTitle)
		LCDTitle->setText(text);
}

void eWindow::drawTitlebar(gPainter *target)
{
	deco.drawDecoration(target, ePoint(width(), height()));
	target->setForegroundColor(titleBarColor);
	target->fill(eRect(titleOffsetX, titleOffsetY, width()-titleOffsetX-titleBorderY, titleHeight));
	target->flush();

	target->setBackgroundColor(titleBarColor);
	target->setForegroundColor(fontColor);
	target->setFont( eSkin::getActive()->queryFont("eWindow.TitleBar") );
	target->renderText(eRect(titleOffsetX, titleOffsetY, width()-titleOffsetX-titleBorderY, titleHeight), text);
	target->flush();
}

void eWindow::recalcClientRect()
{
	clientrect=eRect(borderLeft, borderTop, size.width()-borderLeft-borderRight, size.height()-borderTop-borderBottom);
}

int eWindow::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedText:
			redraw(eRect(0, 0, width(), borderTop));
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
