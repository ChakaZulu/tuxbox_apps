#ifndef __enigma_lcd_h
#define __enigma_lcd_h

#include <core/gui/ewidget.h>
#include <core/gui/eprogress.h>
#include <core/gui/elabel.h>
#include <core/gui/multipage.h>

class eService;
class eZapLCDMain;
class eZapLCDMenu;
class eZapLCDScart;
class eZapLCDStandby;
class eZapLCDShutdown;
class eServiceReference;

class eZapLCD: public eWidget
{
	static eZapLCD* instance;
public:
	static eZapLCD* getInstance() { return instance; }
	eZapLCDMain* lcdMain;
	eZapLCDMenu* lcdMenu;
	eZapLCDScart* lcdScart;
	eZapLCDStandby *lcdStandby;
	eZapLCDShutdown *lcdShutdown;
	eZapLCD();
	~eZapLCD();
};

class eZapLCDMain: public eWidget
{
	eLabel *Clock;
	eProgress *Volume, *Progress;
	eTimer clocktimer;
	int cur_start,cur_duration;
private:
	void clockUpdate();
	void volumeUpdate(int);
	void serviceChanged(const eServiceReference &, int);
	void leaveService(const eServiceReference &service);
public:
	eLabel *ServiceName;
	void updateProgress(int,int);
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
	eZapLCDScart(eWidget *parent);
};

class eZapLCDStandby: public eWidget
{
public:
	eZapLCDStandby(eWidget *parent);
};

class eZapLCDShutdown: public eWidget
{
public:
	eZapLCDShutdown(eWidget *parent);
};

#endif /* __enigma_lcd_h */
