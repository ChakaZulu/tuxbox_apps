#ifndef __elabel_h
#define __elabel_h

#include <core/gui/ewidget.h>
#include <core/gdi/grc.h>

// Definition Blit Flags
#define BF_ALPHATEST 1

class eLabel: public eWidget
{
protected:
	int blitFlags;
	int flags;
	eTextPara *para;
	int align;
	void invalidate();
	void validate();
	void willHide();
	int eventFilter(const eWidgetEvent &event);
public:
	eLabel(eWidget *parent, int flags=0 /* RS_WRAP */ , int takefocus=0);
	~eLabel();

	void redrawWidget(gPainter *target, const eRect &area);
	void setFlags(int flag);
	int setProperty(const eString &prop, const eString &value);

	eSize getExtend();

	ePoint pixmap_position, text_position;
};

#endif
