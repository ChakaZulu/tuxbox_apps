#ifndef __setup_extra_h
#define __setup_extra_h

#include <setup_window.h>

class eExpertSetup: public eSetupWindow
{
#ifndef DISABLE_NETWORK
	void communication_setup();
	void ngrab_setup();
	void software_update();
#endif
	void factory_reset();
	void rc_setup();
	void colorbuttonsChanged(bool);
	void serialDebugChanged(bool);
	void fastZappingChanged(bool b);
public:
	eExpertSetup();
};

#endif
