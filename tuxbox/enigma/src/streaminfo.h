#ifndef __streaminfo_h
#define __streaminfo_h

#include "ewindow.h"
#include "multipage.h"

class eLabel;
class eMultipage;
struct decoderParameters;

class eStreaminfo: public eWindow
{
	eMultipage mp;
protected:
	void keyUp(int code);
	void keyDown(int code);
public:
	eStreaminfo(int mode=0, decoderParameters *parms=0);
};

#endif
