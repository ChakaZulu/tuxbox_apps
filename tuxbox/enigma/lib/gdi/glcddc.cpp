#include "glcddc.h"
#include "lcd.h"

gLCDDC *gLCDDC::instance;

gLCDDC::gLCDDC(eLCD *lcd): lcd(lcd)
{
	instance=this;

	pixmap=new gPixmap();
	pixmap->x=lcd->size().width();
	pixmap->y=lcd->size().height();
	pixmap->bpp=8;
	pixmap->bypp=1;
	pixmap->stride=lcd->stride();
	pixmap->data=lcd->buffer();
	
	pixmap->colors=256;
	pixmap->clut=0;
}

gLCDDC::~gLCDDC()
{
	delete pixmap;
	delete lcd;
	instance=0;
}

void gLCDDC::exec(gOpcode *o)
{
	switch (o->opcode)
	{
	case gOpcode::flush:
	case gOpcode::end:
		lcd->update();
	default:
		gPixmapDC::exec(o);
		break;
	}
}

gLCDDC *gLCDDC::getInstance()
{
	return instance;
}

