#ifndef __gfbdc_h
#define __gfbdc_h

#include "fb.h"
#include "gpixmap.h"
#include "grc.h"

class gFBDC: public gPixmapDC
{
	fbClass *fb;
	static gFBDC *instance;
	void exec(gOpcode *opcode);
public:
	gFBDC();
	~gFBDC();
	static gFBDC *getInstance();
};


#endif
