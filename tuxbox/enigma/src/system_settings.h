#ifndef __system_settings_h
#define __system_settings_h

#include <setup_window.h>

class eSystemSettings: public eSetupWindow
{
private:
	void osd_settings();
	void osd_language();
	void time_settings();
	void av_settings();
#ifndef DISABLE_HDD
#ifndef DISABLE_FILE
	void harddisc_setup();
#endif
#endif
#ifdef ENABLE_KEYBOARD
	void keyboard_setup();
#endif
#ifdef ENABLE_RFMOD
	void uhf_modulator();
#endif
#ifndef DISABLE_LCD
	void lcd_settings();
#endif
public:
	eSystemSettings();
};

#endif /* __system_settings_h */

