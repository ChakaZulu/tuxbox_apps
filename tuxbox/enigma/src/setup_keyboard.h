#ifdef ENABLE_KEYBOARD
#ifndef __setupkeyboard_h
#define __setupkeyboard_h

#include <lib/gui/ewindow.h>

class eButton;
class eLabel;
class eComboBox;
class eStatusBar;

class eZapKeyboardSetup: public eWindow
{
	eComboBox* mappings;
	eButton *ok;
	eStatusBar *statusbar;
	void okPressed();
	void loadMappings();
public:
	eZapKeyboardSetup();
	~eZapKeyboardSetup();
};

#endif // __setupkeyboard_h

#endif // ENABLE_KEYBOARD
