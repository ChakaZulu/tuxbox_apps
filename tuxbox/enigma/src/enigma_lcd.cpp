#include <time.h>
#include "edvb.h"
#include "enigma_lcd.h"
#include "elabel.h"
#include "dvb.h"
#include "lcd.h"
#include "eprogress.h"
#include "glcddc.h"
#include "font.h"
#include "emessage.h"
#include "eskin.h"

#define ASSIGN(v, t, n)	\
	v =(t*)search(n); if (! v ) { qWarning("skin has undefined element: %s", n); v=new t(this); }

eZapLCD::eZapLCD(): eWidget()
{
	setTarget(gLCDDC::getInstance());
	move(QPoint(0, 0));
	resize(QSize(120, 64));

	mp.addPage(new eZapLCDMain(this));
	mp.first();
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
}

void eZapLCDMain::serviceChanged(eService *service, int)
{
	if (service)
		ServiceName->setText(service->service_name);
	else
		ServiceName->setText("unknown");
}

eZapLCDMain::eZapLCDMain(eWidget *parent): eWidget(parent, 0)
{
	if (eSkin::getActive()->build(this, "enigma_lcd"))
		qFatal("skin load of \"enigma_lcd\" failed");

	ASSIGN(ServiceName, eLabel, "service_name");
	ASSIGN(Clock, eLabel, "clock");

	connect(&clocktimer, SIGNAL(timeout()), SLOT(clockUpdate()));
	connect(eDVB::getInstance(), SIGNAL(volumeChanged(int)), SLOT(volumeUpdate(int)));
	connect(eDVB::getInstance(), SIGNAL(switchedService(eService*,int)), SLOT(serviceChanged(eService*,int)));

	clockUpdate();
}
