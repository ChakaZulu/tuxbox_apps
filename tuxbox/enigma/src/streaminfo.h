#ifndef __streaminfo_h
#define __streaminfo_h

#include <lib/gui/ewindow.h>
#include <lib/gui/multipage.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eLabel;
class eMultipage;
struct decoderParameters;
class eServiceReference;

class eStreaminfo: public eWindow
{
	eMultipage mp;
	eStatusBar statusbar;
	eLabel* descr;
	eListBox<eListBoxEntryMenu>* lb;
	static eStreaminfo *instance;
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eStreaminfo(int mode, const eServiceReference &ref, decoderParameters *parms=0);
	~eStreaminfo();
	static eStreaminfo *getInstance() {return instance;}
	eString getCAName(int casysid, int always);
};

#endif
