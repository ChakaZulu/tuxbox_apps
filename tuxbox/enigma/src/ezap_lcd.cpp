#include <time.h>
#include "edvb.h"
#include "ezap_lcd.h"
#include "elabel.h"
#include "dvb.h"
#include "lcd.h"
#include "eprogress.h"
#include "glcddc.h"
#include "font.h"

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
//"News Gothic Br Medium"
	eLabel *l=new eLabel(this);
	l->setText("EliteDVB");
	l->move(QPoint(50, 0));
	l->resize(QSize(70, 16));

	ServiceName=new eLabel(this);
	ServiceName->setText("");
	ServiceName->setFlags(RS_WRAP);
	ServiceName->move(QPoint(0, 20));
	ServiceName->resize(QSize(120, 32));

	Clock=new eLabel(this);
	Clock->setText("EliteDVB");
	Clock->move(QPoint(78, 50));//44
	Clock->resize(QSize(42, 14));

	connect(&clocktimer, SIGNAL(timeout()), SLOT(clockUpdate()));
	connect(eDVB::getInstance(), SIGNAL(volumeChanged(int)), SLOT(volumeUpdate(int)));
	connect(eDVB::getInstance(), SIGNAL(switchedService(eService*,int)), SLOT(serviceChanged(eService*,int)));

	clockUpdate();
	eDVB::getInstance()->changeVolume(0, +1);
}
