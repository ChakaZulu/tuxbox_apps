#ifndef __enigma_setup_h
#define __enigma_setup_h

#include <setup_window.h>

class eZapSetup: public eSetupWindow
{
private:
	void system_settings();
	void service_organising();
	void service_searching();
#ifndef DISABLE_CI
	void common_interface();
#endif
#ifndef DISABLE_FILE
	void harddisc_setup();
#endif
	void expert_setup();
	void parental_lock();
public:
	eZapSetup();
};

#endif /* __enigma_setup_h */
