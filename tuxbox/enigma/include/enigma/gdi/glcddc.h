#ifndef __glcddc_h
#define __glcddc_h

#include "grc.h"

class eLCD;

class gLCDDC: public gPixmapDC
{
	eLCD *lcd;
	static gLCDDC *instance;
	void exec(gOpcode *opcode);
public:
	gLCDDC(eLCD *lcd);
	~gLCDDC();
	static gLCDDC *getInstance();
};


#endif
