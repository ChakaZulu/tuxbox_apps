#ifndef __setupnetwork_h
#define __setupnetwork_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapNetworkSetup: public eWindow
{
	eButton *type;
	eNumber *inet_address, *inet_netmask, *nameserver, *inet_gateway;
	eButton *ok, *abort;
	eCheckbox *dosetup;
	eStatusBar *statusbar;
private:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
public:
	eZapNetworkSetup();
	~eZapNetworkSetup();
};

#endif
