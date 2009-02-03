#ifndef DISABLE_NETWORK

#ifndef __setupengrab_h
#define __setupengrab_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;
class eTextInputField;

class ENgrabSetup: public eWindow
{
	eButton *type;
	eNumber *inet_address, *srvport;
	eButton *ok, *bServerMAC;
	eStatusBar *statusbar;
	eTextInputField *serverMAC;
private:
	void fieldSelected(int *number);
	void okPressed();
	void detectMAC();
	void init_ENgrabSetup();
public:
	ENgrabSetup();
	~ENgrabSetup();
};
#endif

#endif // DISABLE_NETWORK
