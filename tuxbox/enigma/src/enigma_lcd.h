#ifndef __enigma_lcd_h
#define __enigma_lcd_h

#include "ewidget.h"
#include "eprogress.h"
#include "elabel.h"
#include "multipage.h"

class eService;
class eZapLCDMain;
class eZapLCDMenu;
class eZapLCDScart;

class eZapLCD: public eWidget
{
	static eZapLCD* instance;
public:
	static eZapLCD* getInstance() { return instance; }
	eZapLCDMain* lcdMain;
	eZapLCDMenu* lcdMenu;
	eZapLCDScart* lcdScart;
	eZapLCD();
	~eZapLCD();
};

class eZapLCDMain: public eWidget
{
	eLabel *Clock, *ServiceName;
	eProgress *Volume;
	eTimer clocktimer;
private:
	void clockUpdate();
	void volumeUpdate(int);
	void serviceChanged(eService *, int);
public:
	eZapLCDMain(eWidget *parent);
};

class eZapLCDMenu: public eWidget
{
public:
	eLabel *Title;
	eWidget *Element;
	eZapLCDMenu(eWidget *parent);
};

class eZapLCDScart: public eWidget
{
public:
	eLabel *Title, *Scart;
	eZapLCDScart(eWidget *parent);
};

#endif /* __enigma_lcd_h */
