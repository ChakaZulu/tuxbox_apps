#ifndef __setupvideo_h
#define __setupvideo_h

#include <core/gui/ewindow.h>
#include <core/gui/listbox.h>
#include <core/gui/statusbar.h>
#include <core/driver/eavswitch.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapVideoSetup: public eWindow
{
	eButton *abort, *ok;
	eStatusBar *status;

	eListBox<eListBoxEntryText> *colorformat, *pin8;

	unsigned int v_colorformat, v_pin8;
private:
	void okPressed();
	void abortPressed();

public:
	eZapVideoSetup();
	~eZapVideoSetup();
};

#endif
