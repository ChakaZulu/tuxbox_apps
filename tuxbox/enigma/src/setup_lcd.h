#ifndef __setuplcd_h
#define __setuplcd_h

#include <core/gui/ewindow.h>
#include <core/gui/statusbar.h>

class eLabel;
class eButton;
class eSlider;

class eZapLCDSetup: public eWindow
{
	eSlider *p_brightness, *p_contrast, *p_standby;
	eLabel *bbrightness, *bcontrast, *bstandby;
	eStatusBar* statusbar;
	
	int lcdbrightness;
	int lcdcontrast;
	int lcdstandby;
	                	
	eButton *ok, *abort;
	void okPressed();
	void abortPressed();
	void brightnessChanged( int );
	void contrastChanged( int );
	void standbyChanged( int );
	void update();
public:
	eZapLCDSetup();
	~eZapLCDSetup();
private:
};

#endif
