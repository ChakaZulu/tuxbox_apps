#ifndef __elabel_h
#define __elabel_h

#include <core/gui/ewidget.h>
#include "qstring.h"
#include <core/gdi/grc.h>

class eLabel: public eWidget
{
//	Q_OBJECT
protected:
	int flags;
	eTextPara *para;
	ePoint pixmap_position, text_position;
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
};

#endif
