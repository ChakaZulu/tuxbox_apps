#ifndef __setuposd_h
#define __setuposd_h

#include "ewindow.h"
class eCheckbox;
class eButton;

class eZapOsdSetup: public eWindow
{
	eCheckbox* showOSDOnEITUpdate;
	eButton *ok, *abort;
private:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
public:
	eZapOsdSetup();
	~eZapOsdSetup();
private:
};

#endif
