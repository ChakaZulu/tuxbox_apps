#include "gfbdc.h"
#include <core/system/init.h>

gFBDC *gFBDC::instance;

gFBDC::gFBDC()
{
	instance=this;
	fb=new fbClass;

	if (!fb->Available())
		eFatal("no framebuffer available");

	fb->SetMode(720, 576, 8);
	for (int y=0; y<576; y++)																		 // make whole screen transparent
		memset(fb->lfb+y*fb->Stride(), 0x00, fb->Stride());

	pixmap=new gPixmap();
	pixmap->x=720;
	pixmap->y=576;
	pixmap->bpp=8;
	pixmap->bypp=1;
	pixmap->stride=fb->Stride();
	pixmap->data=fb->lfb;
	
	pixmap->clut.colors=256;
	pixmap->clut.data=new gRGB[pixmap->clut.colors];
}

gFBDC::~gFBDC()
{
	delete pixmap;
	delete fb;
	instance=0;
}

void gFBDC::exec(gOpcode *o)
{
	switch (o->opcode)
	{
	case gOpcode::setPalette:
	{
		for (int i=o->parm.setPalette->palette->start; i<o->parm.setPalette->palette->colors; i++)
		{
			fb->CMAP()->red[i]=o->parm.setPalette->palette->data[i].r<<8;
			fb->CMAP()->green[i]=o->parm.setPalette->palette->data[i].g<<8;
			fb->CMAP()->blue[i]=o->parm.setPalette->palette->data[i].b<<8;
			fb->CMAP()->transp[i]=o->parm.setPalette->palette->data[i].a<<8;
			if (!fb->CMAP()->red[i])
				fb->CMAP()->red[i]=0x100;
		}
		fb->PutCMAP();
		gPixmapDC::exec(o);
		break;
	}
	default:
		gPixmapDC::exec(o);
		break;
	}
}

gFBDC *gFBDC::getInstance()
{
	return instance;
}

eAutoInitP0<gFBDC> init_gFBDC(1, "GFBDC");
