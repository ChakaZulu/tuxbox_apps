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
	eStatusBar* statusbar;
	eCheckbox* inverted, *shortnames;
	
	int lcdbrightness;
	int lcdcontrast;
	int lcdstandby;
	unsigned char lcdinverted;

	eButton *ok, *abort;
	void okPressed();
	void abortPressed();
	void brightnessChanged( int );
	void contrastChanged( int );
	void standbyChanged( int );
	void invertedChanged( int );
	void update(int brightness, int contrast);
public:
	eZapLCDSetup();
	~eZapLCDSetup();
private:
};

#endif
