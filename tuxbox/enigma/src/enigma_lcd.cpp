#include <time.h>
#include "edvb.h"
#include "enigma_lcd.h"
#include "dvb.h"
#include "lcd.h"
#include "glcddc.h"
#include "font.h"
#include "emessage.h"
#include "eskin.h"

eZapLCD* eZapLCD::instance;

eZapLCD::eZapLCD(): eWidget()
{
	instance = this;
	setTarget(gLCDDC::getInstance());
	move(ePoint(0, 0));
	resize(eSize(120, 64));

	lcdMain = new eZapLCDMain(this);
	lcdMenu = new eZapLCDMenu(this);
	lcdScart = new eZapLCDScart(this);
	lcdMenu->hide();
	lcdScart->hide();
}

eZapLCD::~eZapLCD()
{
	delete lcdMain;
	delete lcdMenu;
	delete lcdScart;
}

void eZapLCDMain::clockUpdate()
{
	time_t c=time(0)+eDVB::getInstance()->time_difference;
	tm *t=localtime(&c);
	if (t)
	{
		QString s;
		s.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
		clocktimer.start((70-t->tm_sec)*1000);
		Clock->setText(s);
	} else
	{
		Clock->setText("--:--");
		clocktimer.start(60000);
	}
}

void eZapLCDMain::volumeUpdate(int vol)
{
	Volume->setPerc((63-vol)*100/63);
}

void eZapLCDMain::serviceChanged(eService *service, int)
{
	if (service)
		ServiceName->setText(service->service_name.c_str());
	else
		ServiceName->setText("unknown");
}

eZapLCDMain::eZapLCDMain(eWidget *parent): eWidget(parent, 0)
{
	if (eSkin::getActive()->build(this, "enigma_lcd_main"))
		qFatal("skin load of \"enigma_lcd\" failed");

	ASSIGN(Volume, eProgress, "volume_bar");
	ASSIGN(ServiceName, eLabel, "service_name");
	ASSIGN(Clock, eLabel, "clock");
	Volume->show();
	
/*	connect(&clocktimer, SIGNAL(timeout()), SLOT(clockUpdate()));
	connect(eDVB::getInstance(), SIGNAL(timeUpdated()), SLOT(clockUpdate()));
	connect(eDVB::getInstance(), SIGNAL(volumeChanged(int)), SLOT(volumeUpdate(int)));
	connect(eDVB::getInstance(), SIGNAL(switchedService(eService*,int)), SLOT(serviceChanged(eService*,int)));*/
	CONNECT(clocktimer.timeout, eZapLCDMain::clockUpdate);
	CONNECT(eDVB::getInstance()->timeUpdated, eZapLCDMain::clockUpdate);
	CONNECT(eDVB::getInstance()->volumeChanged, eZapLCDMain::volumeUpdate);
	CONNECT(eDVB::getInstance()->switchedService, eZapLCDMain::serviceChanged);
	clockUpdate();
}

eZapLCDMenu::eZapLCDMenu(eWidget *parent): eWidget(parent, 0)
{	
	if (eSkin::getActive()->build(this, "enigma_lcd_menu"))
		qFatal("skin load of \"enigma_lcd_menu\" failed");

	ASSIGN(Title, eLabel, "title");
	ASSIGN(Element, eLabel, "element");
}

eZapLCDScart::eZapLCDScart(eWidget *parent): eWidget(parent, 0)
{	
	if (eSkin::getActive()->build(this, "enigma_lcd_scart"))
		qFatal("skin load of \"enigma_lcd_scart\" failed");

	ASSIGN(Title, ePixmap, "enigma_logo");
	ASSIGN(Scart, ePixmap, "lcd_scart");
}
