#ifndef __setuplcd_h
#define __setuplcd_h

#include <core/gui/ewindow.h>

class eProgress;
class eButton;

class eZapLCDSetup: public eWindow
{
	eProgress *p_brightness, *p_contrast, *p_standby;
	eButton *bbrightness, *bcontrast, *bstandby;
	
	int lcdbrightness;
	int lcdcontrast;
	int lcdstandby;
	                	
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
