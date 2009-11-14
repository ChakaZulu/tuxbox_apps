#ifndef __setuptimezone_h
#define __setuptimezone_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eLabel;
class eButton;
class eComboBox;
class eListBoxEntryText;
class eCheckbox;

class eZapTimeZoneSetup: public eWindow
{
	int errLoadTimeZone;
	
	eComboBox* timeZone;
	bool showHint;
	void init_eZapTimeZoneSetup();
private:
	void okPressed();

	int loadTimeZones();
	char *cmdTimeZones();
public:
	eZapTimeZoneSetup(bool showHint=true);
	~eZapTimeZoneSetup();
	void setTimeZone();
};

#endif
