#ifndef __setupnetwork_h
#define __setupnetwork_h

#include "ewindow.h"

class eNumber;
class eButton;
class eCheckbox;

class eZapNetworkSetup: public eWindow
{
	Q_OBJECT
	eNumber *ip, *netmask, *dns, *gateway;
	eButton *ok, *abort;
	eCheckbox *dosetup;
private slots:
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
