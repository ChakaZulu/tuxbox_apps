#ifndef __setuposd_h
#define __setuposd_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/slider.h>

class eCheckbox;
class eButton;

class eZapOsdSetup: public eWindow
{
	eSlider *sAlpha, *sBrightness, *sGamma;
	eCheckbox* showOSDOnEITUpdate;
	eCheckbox* showConsoleOnFB;
	eStatusBar* statusbar;

	eButton *ok, *abort;
	int alpha, brightness, gamma;
private:
	void alphaChanged( int );
	void brightnessChanged( int );
	void gammaChanged( int );
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
public:
	eZapOsdSetup();
	~eZapOsdSetup();
private:
};

#endif
