#ifndef __enigma_lcd_h
#define __enigma_lcd_h

#include "ewidget.h"
#include "multipage.h"
#include "qtimer.h"

class eLabel;
class eService;
class eProgress;
class eZapLCDMain;
class eZapLCDMenu;

class eZapLCD: public eWidget
{
	Q_OBJECT
	static eZapLCD* instance;
public:
	static eZapLCD* getInstance() { return instance; }
	eZapLCDMain* lcdMain;
	eZapLCDMenu* lcdMenu;
	eZapLCD();
};

class eZapLCDMain: public eWidget
{
	Q_OBJECT
	eLabel *Clock, *ServiceName;
	eProgress *Volume;
	QTimer clocktimer;
private slots:
	void clockUpdate();
	void volumeUpdate(int);
	void serviceChanged(eService *, int);
public:
	eZapLCDMain(eWidget *parent);
};

class eZapLCDMenu: public eWidget
{
	Q_OBJECT
private slots:

public:
	eLabel *Title;
	eWidget *Element;
	eZapLCDMenu(eWidget *parent);
};

#endif /* __enigma_lcd_h */
