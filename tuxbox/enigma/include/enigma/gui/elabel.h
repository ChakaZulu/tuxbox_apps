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
	virtual void redrawWidget(gPainter *target, const eRect &area);
	int setProperty(const eString &prop, const eString &value);
public:
	eLabel(eWidget *parent, int flags=0 /* RS_WRAP */ , int takefocus=0 );
	~eLabel();

	void setFlags(int flag);
	void setAlign(int align);

	eSize getExtend();

	ePoint pixmap_position, text_position;
};

#endif
