#ifndef __time_settings_h
#define __time_settings_h

#include <setup_window.h>

class eTimeSettings: public eSetupWindow
{
private:
	void time_zone();
	void time_correction();
public:
	eTimeSettings();
};

#endif /* __time_settings_h */

