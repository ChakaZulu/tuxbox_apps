#ifndef __software_update_h
#define __software_update_h

#include <setup_window.h>

class eSoftwareUpdate: public eSetupWindow
{
private:
#ifndef DISABLE_NETWORK
	void internet_update();
	void manual_update();
#ifdef ENABLE_FLASHTOOL
	void flash_tool();
#endif
#endif
//	void satellite_update();
public:
	eSoftwareUpdate();
};

#endif /* __software_update_h */
