#ifndef __ewindow_h
#define __ewindow_h

#include "ewidget.h"

class eWindow: public eWidget
{
	Q_OBJECT

	gPixmap *iTopLeft, *iTop, 
			*iTopRight, *iLeft, *iRight, 
			*iBottomLeft, *iBottom, *iBottomRight;
protected:
	int titleSize, border, titleOffsetX, titleOffsetY, titleFontSize;
	void redrawWidget(gPainter *target, const QRect &where);
	void drawTitlebar(gPainter *target);
  void OnFontSizeChanged(int NewFontSize);
	void recalcClientRect();
	int eventFilter(const eWidgetEvent &event);
public:
	eWindow(int takefocus=0, eWidget* lcdTitle=0, eWidget* lcdElement=0 );
	~eWindow();
};

#endif
