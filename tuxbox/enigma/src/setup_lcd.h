#ifndef DISABLE_LCD

#ifndef __setuplcd_h
#define __setuplcd_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eLabel;
class eButton;
class eSlider;
class eCheckbox;

class eZapLCDSetup: public eWindow
{
	eSlider *p_brightness, *p_contrast, *p_standby;
	eLabel *bbrightness, *bcontrast, *bstandby;
	eCheckbox* inverted;
	
	int lcdbrightness;
	int lcdcontrast;
	int lcdstandby;

	void okPressed();
	int eventHandler( const eWidgetEvent&);
	void brightnessChanged( int );
	void contrastChanged( int );
	void standbyChanged( int );
	void invertedChanged( int );
	void update(int brightness, int contrast);
	void init_eZapLCDSetup();
public:
	eZapLCDSetup();
	~eZapLCDSetup();
private:
};

#endif

#endif //DISABLE_LCD
