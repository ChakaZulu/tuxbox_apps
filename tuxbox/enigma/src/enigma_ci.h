#ifndef __enigmaci_h
#define __enigmaci_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

class eDVBCI;

class enigmaCI: public eWindow
{
	eButton *ok,*reset,*init,*app;
	eStatusBar *status;
	eDVBCI *DVBCI;

private:
	void okPressed();
	void abortPressed();
	void resetPressed();
	void initPressed();
	void appPressed();
	void updateCIinfo(const char*);

public:
	enigmaCI();
	~enigmaCI();
};

#endif
