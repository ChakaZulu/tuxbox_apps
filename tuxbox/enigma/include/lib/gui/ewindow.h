#ifndef __ewindow_h
#define __ewindow_h

#include <core/gui/ewidget.h>
#include <core/gui/decoration.h>

/**
 * \brief A (decorated) top level widget.
 *
 * An eWindow is whats actually seen as a window. It's top level (thus you cannot specify a parent),
 * and may have a (skinned) decoration. It's clientrect is usually a bit smaller since it has a titlebar
 * and a border.
 *
 */
class eWindow: public eWidget
{
	eDecoration deco;
	gColor fontColor, titleBarColor;
protected:
	int borderTop, borderLeft, borderBottom, borderRight;
	int titleOffsetX, titleOffsetY, titleFontSize, titleHeight, titleBorderY;
	void redrawWidget(gPainter *target, const eRect &where);
	void eraseBackground(gPainter *target, const eRect &where);
	void drawTitlebar(gPainter *target);
	void recalcClientRect();
	int eventHandler(const eWidgetEvent &event);
	void willShow();
	void willHide();
public:
	eRect getTitleBarRect();
	/**
	 * \brief Constructs the window
	 *
	 * \arg takefocus the \c eWidget::eWidget takefocus parameter. You don't need to set it if just
	 * one widget \e inside the parent needs focus. That widget can apply for it by itself.
	 */
	eWindow(int takefocus=0);
	
	/**
	 * destructs the window.
	 */
	~eWindow();
};

#endif
