#ifndef __streaminfo_h
#define __streaminfo_h

#include <core/gui/ewindow.h>
#include <core/gui/multipage.h>

class eLabel;
class eMultipage;
struct decoderParameters;

class eStreaminfo: public eWindow
{
	eMultipage mp;
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eStreaminfo(int mode=0, decoderParameters *parms=0);
	~eStreaminfo();
};

#endif
