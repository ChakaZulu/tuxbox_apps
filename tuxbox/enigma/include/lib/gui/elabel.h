#ifndef __elabel_h
#define __elabel_h

#include "ewidget.h"
#include "qstring.h"
#include "grc.h"

class eLabel: public eWidget
{
//	Q_OBJECT
protected:
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
	int setProperty(const QString &prop, const QString &value);

	eSize getExtend();
};

#endif
