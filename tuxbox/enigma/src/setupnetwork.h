#ifndef __setupnetwork_h
#define __setupnetwork_h

#include <core/gui/ewindow.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapNetworkSetup: public eWindow
{
	eNumber *ip, *netmask, *dns, *gateway;
	eButton *ok, *abort;
	eCheckbox *dosetup;
private:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
protected:
	int eventFilter(const eWidgetEvent &event);
public:
	eZapNetworkSetup();
	~eZapNetworkSetup();
};

#endif
