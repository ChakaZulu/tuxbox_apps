#ifndef __core_gui_decoration_h
#define __core_gui_decoration_h

#include <core/base/epoint.h>
class gPixmap;
class gPainter;

class eDecoration
{
	gPixmap *iTopLeft, *iTop,
			*iTopRight, *iLeft, *iRight, 
			*iBottomLeft, *iBottom, *iBottomRight;
			
public:
	operator bool() { return iTopLeft || iTop || iTopRight || iLeft || iRight || iBottomLeft || iBottom || iBottomRight; }
	
	eDecoration();
	void load(const char *basename);

	void drawDecoration(gPainter *target, ePoint size);
	int borderTop, borderLeft, borderBottom, borderRight;
};

#endif
