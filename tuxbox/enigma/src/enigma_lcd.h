#ifndef __enigma_lcd_h
#define __enigma_lcd_h

#include <lib/gui/ewidget.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/elabel.h>
#include <lib/gui/multipage.h>

class eService;
class eZapLCDMain;
class eZapLCDMenu;
class eZapLCDScart;
class eZapLCDStandby;
class eZapLCDShutdown;
class eServiceReferenceDVB;

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
	eProgress *Volume;
	int cur_start,cur_duration;
private:
	void volumeUpdate(int);
	void leaveService(const eServiceReferenceDVB &service);
public:
	eLabel *Clock;
	eProgress *Progress;
	void setServiceName(eString name);
	eLabel *ServiceName;
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
	eProgress* volume;
public:
	eZapLCDScart(eWidget *parent);
	void volumeUpdate(int);
};

class eZapLCDStandby: public eWidget
{
public:
	eLabel *Clock;
	eZapLCDStandby(eWidget *parent);
};

class eZapLCDShutdown: public eWidget
{
public:
	eZapLCDShutdown(eWidget *parent);
};

#endif /* __enigma_lcd_h */
