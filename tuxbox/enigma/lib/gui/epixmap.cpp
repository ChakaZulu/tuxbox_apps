#include "epixmap.h"
#include "eskin.h"
#include "init.h"

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

static eWidget *create_ePixmap(eWidget *parent)
{
	return new ePixmap(parent);
}

class ePixmapSkinInit
{
public:
	ePixmapSkinInit()
	{
		eSkin::addWidgetCreator("ePixmap", create_ePixmap);
	}
	~ePixmapSkinInit()
	{
		eSkin::removeWidgetCreator("ePixmap", create_ePixmap);
	}
};

eAutoInitP0<ePixmapSkinInit,3> init_ePixmapSkinInit("ePixmap");
