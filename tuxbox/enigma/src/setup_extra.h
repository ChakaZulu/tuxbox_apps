#ifndef __setup_extra_h
#define __setup_extra_h

#include <setup_window.h>

class eExpertSetup: public eSetupWindow
{
#ifndef DISABLE_FILE
	eListBoxEntryMulti *record_split_size;
	void selChanged(eListBoxEntryMenu* e);
	eListBoxEntryMulti *timeroffsetstart;
	eListBoxEntryMulti *timeroffsetend;
	eListBoxEntryMulti *timerenddefaultaction;
	void startoffsetChanged(eListBoxEntryMenu* e);
	void endoffsetChanged(eListBoxEntryMenu* e);
	void timerenddefaultactionChanged(eListBoxEntryMenu* e);
#endif
#ifndef DISABLE_NETWORK
	void communication_setup();
	void ngrab_setup();
	void software_update();
#endif
	void factory_reset();
	void rc_setup();
#ifndef DISABLE_HDD
#ifndef DISABLE_FILE
	void swapmanager();
#endif
#endif

#ifndef TUXTXT_CFG_STANDALONE
	void tuxtxtCachingChanged(bool);
#endif
	void reinitializeHTTPServer(bool);
	void fastZappingChanged(bool b);
	void init_eExpertSetup();
	void fileToggle(bool newState, const char* filename);
public:
	eExpertSetup();
};

#endif
