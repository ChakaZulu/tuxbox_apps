#ifndef __setuprc_h
#define __setuprc_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eLabel;
class eButton;
class eSlider;

class eZapRCSetup: public eWindow
{
	eSlider *srrate, *srdelay;
	eLabel *lrrate, *lrdelay;
	eStatusBar* statusbar;
	
	int rdelay;
	int rrate;
	                	
	eButton *ok, *abort;
	void okPressed();
	void abortPressed();
	void repeatChanged( int );
	void delayChanged( int );
	void update();
public:
	eZapRCSetup();
	~eZapRCSetup();
private:
};

#endif
