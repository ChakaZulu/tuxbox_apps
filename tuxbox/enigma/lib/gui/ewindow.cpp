#include <lib/gui/ewindow.h>
#include <lib/gdi/grc.h>
#include <lib/gui/eskin.h>
#include <lib/system/init.h>
#include <lib/gdi/epng.h>
#include <lib/gui/elabel.h>
#include <lib/gui/guiactions.h>
#include <lib/gdi/font.h>

int eWindow::globCancel = eWindow::ON;

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

	titleOffsetLeft=eSkin::getActive()->queryValue("eWindow.titleOffsetLeft", 0);
	titleOffsetRight=eSkin::getActive()->queryValue("eWindow.titleOffsetRight", 0);
	titleOffsetTop=eSkin::getActive()->queryValue("eWindow.titleOffsetTop", 0);
	titleHeight=eSkin::getActive()->queryValue("eWindow.titleHeight", titleFontSize+10);
	titleFontSize=eSkin::getActive()->queryValue("eWindow.titleFontSize", 20);

	font = eSkin::getActive()->queryFont("eWindow.Childs");

	addActionMap(&i_cursorActions->map);
}

eWindow::~eWindow()
{
}

eRect eWindow::getTitleBarRect()
{
	eRect rc;
	rc.setLeft( deco.borderLeft > titleOffsetLeft ? deco.borderLeft : titleOffsetLeft );
	rc.setTop( titleOffsetTop );
	rc.setRight( width() - ( deco.borderRight > titleOffsetRight ? deco.borderRight : titleOffsetRight ) );
	rc.setBottom( rc.top() + (titleHeight?titleHeight:deco.borderTop) );  // deco.borderTop sucks...

	return rc;
}

void eWindow::redrawWidget(gPainter *target, const eRect &where)
{
	if ( deco )  // then draw Deco
		deco.drawDecoration(target, ePoint(width(), height()));

	drawTitlebar(target);
}

void eWindow::eraseBackground(gPainter *target, const eRect &clip)
{
	target->clip(getClientRect());
	target->clear();
	target->clippop();
}

void eWindow::drawTitlebar(gPainter *target)
{
	eRect rc = getTitleBarRect();
  target->clip( rc );
	if ( titleHeight )
	{
		target->setForegroundColor(titleBarColor);
		target->fill( rc );
	}

	eTextPara *p = new eTextPara( rc );
	p->setFont( eSkin::getActive()->queryFont("eWindow.TitleBar") );
	p->renderString( text );
	target->setBackgroundColor(titleBarColor);
	target->setForegroundColor(fontColor);
	target->renderPara( *p );
	p->destroy();
  target->clippop();
}

void eWindow::recalcClientRect()
{
	clientrect=eRect( borderLeft, borderTop, width() - (borderLeft+borderRight), height() - ( borderTop+borderBottom) );
}

int eWindow::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedText:
		{
			redraw(getTitleBarRect());
			return 1;
		}
    
		case eWidgetEvent::evtAction:
			if (globCancel && (event.action == &i_cursorActions->cancel) && in_loop)	// hack
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
	if (LCDTitle)
		LCDTitle->setText(text);
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
