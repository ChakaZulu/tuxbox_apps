#ifndef __setuposd_h
#define __setuposd_h

#include <core/gui/ewindow.h>
#include <core/gui/statusbar.h>

class eCheckbox;
class eButton;

class eZapOsdSetup: public eWindow
{
	eCheckbox* showOSDOnEITUpdate;
	eCheckbox* showConsoleOnFB;
	eStatusBar* statusbar;

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
