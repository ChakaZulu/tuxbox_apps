#ifndef __setupvideo_h
#define __setupvideo_h

#include "ewindow.h"
#include "eavswitch.h"

class eNumber;
class eButton;
class eCheckbox;

class eZapVideoSetup: public eWindow
{
	Q_OBJECT
	eButton *colorformat, *pin8, *abort, *ok;
	
	unsigned int v_pin8; 		// 0: 4:3 Letterboxed, 1: 4:3 panscan, 2: 16:9 w/ pin8 signal
	eAVColorFormat v_colorformat;		// eColorFormat ...
	void setPin8(int w);
	void setColorFormat(eAVColorFormat w);

private slots:
	void okPressed();
	void abortPressed();
	
	void toggleColorformat();
	void togglePin8();
protected:
	int eventFilter(const eWidgetEvent &event);
public:
	eZapVideoSetup(eWidget *lcdTitle=0, eWidget *lcdElement=0);
	~eZapVideoSetup();
};

#endif
