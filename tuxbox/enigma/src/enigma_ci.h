#ifndef __enigmaci_h
#define __enigmaci_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

class enigmaCI: public eWindow
{
	eButton *ok,*reset,*init,*app;
	eStatusBar *status;

private:
	void okPressed();
	void abortPressed();
	void resetPressed();
	void initPressed();
	void appPressed();

public:
	enigmaCI();
	~enigmaCI();
};

#endif
