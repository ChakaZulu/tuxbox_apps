#ifndef __setuplcd_h
#define __setuplcd_h

#include <core/gui/ewindow.h>

class eProgress;
class eButton;
class eNumber;

class eZapLCDSetup: public eWindow
{
	eNumber *brightness, *contrast;
	eProgress *p_brightness, *p_contrast;
	eButton *bbrightness, *bcontrast;
	
	int lcdbrightness;
	int lcdcontrast;
                	
	eButton *ok, *abort;
protected:
	int eventHandler(const eWidgetEvent &event);
	void update();
private:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
public:
	eZapLCDSetup();
	~eZapLCDSetup();
private:
};

#endif
