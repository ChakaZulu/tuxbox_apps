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
	void validate( const eSize* s=0 );
	void invalidate();
	void willHide();
	int eventFilter(const eWidgetEvent &event);
	virtual void redrawWidget(gPainter *target, const eRect &area);
	int setProperty(const eString &prop, const eString &value);
	int yOffs;
public:
	enum { flagVCenter = 64 };
	eLabel(eWidget *parent, int flags=0 /* RS_WRAP */ , int takefocus=0 );
	~eLabel();

	void setFlags(int flags);
	void removeFlags(int flags);
	void setAlign(int align);

	eSize getExtend();
	ePoint getLeftTop();

	ePoint pixmap_position, text_position;
};

#endif
