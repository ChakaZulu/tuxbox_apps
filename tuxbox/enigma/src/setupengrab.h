#ifndef __setupengrab_h
#define __setupengrab_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

  class ENgrabSetup: public eWindow
{
	eButton *type;
	eNumber *inet_address, *srvport;
	eButton *ok, *abort;
	eStatusBar *statusbar;
private:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
public:
	ENgrabSetup();
	~ENgrabSetup();
};
#endif
