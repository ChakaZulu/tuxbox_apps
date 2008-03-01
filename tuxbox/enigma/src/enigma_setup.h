#ifndef __enigma_setup_h
#define __enigma_setup_h

#include <src/setup_window.h>

class eZapSetup: public eSetupWindow
{
private:
	void system_settings();
	void service_organising();
	void service_searching();
#ifndef DISABLE_CI
	void common_interface();
#endif
	void expert_setup();
	void parental_lock();
#ifndef DISABLE_HDD
#ifndef DISABLE_FILE
	void timeshift_setup();
#endif
#endif
public:
	eZapSetup();
};

#endif /* __enigma_setup_h */
