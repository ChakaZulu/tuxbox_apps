#ifndef __eprogress_h
#define __eprogress_h

#include "ewidget.h"
#include "grc.h"

class eProgress: public eWidget
{
//	Q_OBJECT
	int perc, border;
	gColor left, right;
public:
	eProgress(eWidget *parent);
	~eProgress();
	
	void setPerc(int perc);
	void redrawWidget(gPainter *target, const eRect &area);
	int setProperty(const QString &prop, const QString &value);
};

#endif
