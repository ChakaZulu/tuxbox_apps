#ifndef __setuposd_h
#define __setuposd_h

#include <lib/gui/ewindow.h>

class eCheckbox;
class eButton;
class eStatusBar;
class eSlider;

class eZapOsdSetup: public eWindow
{
	eSlider *sAlpha, *sBrightness, *sGamma;
	eStatusBar *statusbar;

	eButton *pluginoffs, *skin, *ok;
	eCheckbox *simpleMainMenu;
	int alpha, brightness, gamma;
private:
	int eventHandler(const eWidgetEvent&);
	void skinPressed();
	void alphaChanged( int );
	void brightnessChanged( int );
	void gammaChanged( int );
	void pluginPositionPressed();
	void okPressed();
	void init_eZapOsdSetup();
public:
	eZapOsdSetup();
	~eZapOsdSetup();
private:
};

#endif
