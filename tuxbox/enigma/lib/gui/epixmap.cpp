#include "epixmap.h"

ePixmap::ePixmap(eWidget *parent): eWidget(parent)
{
	position=QPoint(0, 0);
}

ePixmap::~ePixmap()
{
}

void ePixmap::redrawWidget(gPainter *paint, const QRect &area)
{
	if (pixmap)
		paint->blit(*pixmap, position);
}
