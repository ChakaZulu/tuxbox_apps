#ifndef __setupvideo_h
#define __setupvideo_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>
#include <lib/driver/eavswitch.h>

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
