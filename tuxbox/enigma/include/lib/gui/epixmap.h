#ifndef __epixmap_h
#define __epixmap_h

#include "ewidget.h"
class gPixmap;

class ePixmap: public eWidget
{
	QPoint position;
public:
	ePixmap(eWidget *parent);
	~ePixmap();
	
	void redrawWidget(gPainter *paint, const QRect &area);
};

#endif
