#ifndef __core_gui_decoration_h
#define __core_gui_decoration_h

#include <core/base/erect.h>
#include <core/base/estring.h>
class gPixmap;
class gPainter;

class eDecoration
{
	gPixmap *iTopLeft, *iTop,
			*iTopRight, *iLeft, *iRight, 
			*iBottomLeft, *iBottom, *iBottomRight;

	eString	basename;
public:
	operator bool() { return iTopLeft || iTop || iTopRight || iLeft || iRight || iBottomLeft || iBottom || iBottomRight; }
	
	eDecoration();
	bool load(const eString& basename);

	void drawDecoration(gPainter *target, ePoint size);
	int borderTop, borderLeft, borderBottom, borderRight;
};

#endif
