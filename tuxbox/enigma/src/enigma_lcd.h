#ifndef __enigma_lcd_h
#define __enigma_lcd_h

#include "ewidget.h"
#include "multipage.h"
#include "qtimer.h"

class eLabel;
class eService;
class eProgress;

class eZapLCD: public eWidget
{
	Q_OBJECT
	eMultipage mp;
public:
	eZapLCD();
};

class eZapLCDMain: public eWidget
{
	Q_OBJECT
	eLabel *Clock, *ServiceName;
	QTimer clocktimer;
private slots:
	void clockUpdate();
	void volumeUpdate(int);
	void serviceChanged(eService *, int);
public:
	eZapLCDMain(eWidget *parent);
};

#endif /* __enigma_lcd_h */
