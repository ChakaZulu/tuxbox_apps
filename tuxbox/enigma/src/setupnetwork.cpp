#include <netinet/in.h>
#include "setupnetwork.h"
#include "elabel.h"
#include "edvb.h"
#include "enumber.h"
#include "ebutton.h"
#include "echeckbox.h"
#include "rc.h"
#include "eskin.h"

static void unpack(__u32 l, int *t)
{
	for (int i=0; i<4; i++)
		*t++=(l>>((3-i)*8))&0xFF;
}

static void pack(__u32 &l, int *t)
{
	l=0;
	for (int i=0; i<4; i++)
		l|=(*t++)<<((3-i)*8);
}

eZapNetworkSetup::eZapNetworkSetup(eWidget *lcdTitle, eWidget *lcdElement ):
eWindow(0, lcdTitle, lcdElement)
{
	setText("Network Setup");
	move(QPoint(150, 136));
	resize(QSize(420, 290));

	__u32 sip=ntohl(0x0a000061), snetmask=ntohl(0xFF000000), sdns=ntohl(0x7f000001), sgateway=ntohl(0x7f000001);
	int de[4];
	int sdosetup=0;
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eDVB::getInstance()->config.getKey("/elitedvb/network/ip", sip);
	eDVB::getInstance()->config.getKey("/elitedvb/network/netmask", snetmask);
	eDVB::getInstance()->config.getKey("/elitedvb/network/dns", sdns);
	eDVB::getInstance()->config.getKey("/elitedvb/network/gateway", sgateway);
	eDVB::getInstance()->config.getKey("/elitedvb/network/dosetup", sdosetup);

	eLabel *l=new eLabel(this);
	l->setText("IP:");
	l->move(QPoint(10, 0));
	l->resize(QSize(150, fd+4));

	unpack(sip, de);
	ip=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	ip->move(QPoint(160, 0));
	ip->resize(QSize(200, fd+4));

	l=new eLabel(this);
	l->setText("Netmask:");
	l->move(QPoint(10, 40));
	l->resize(QSize(150, fd+4));

	unpack(snetmask, de);
	netmask=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	netmask->move(QPoint(160, 40));
	netmask->resize(QSize(200, fd+4));
	
	l=new eLabel(this);
	l->setText("Nameserver:");
	l->move(QPoint(10, 80));
	l->resize(QSize(150, fd+4));

	unpack(sdns, de);
	dns=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	dns->move(QPoint(160, 80));
	dns->resize(QSize(200, fd+4));

	l=new eLabel(this);
	l->setText("Gateway:");
	l->move(QPoint(10, 120));
	l->resize(QSize(150, fd+4));

	unpack(sgateway, de);
	gateway=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	gateway->move(QPoint(160, 120));
	gateway->resize(QSize(200, fd+4));

	connect(ip, SIGNAL(selected(int*)), SLOT(fieldSelected(int*)));

	l=new eLabel(this);
	l->setText("Configure Network");
	l->move(QPoint(130, 160));
	l->resize(QSize(240, fd+4));

	dosetup=new eCheckbox(this, sdosetup, fd, l);
	dosetup->move(QPoint(100, 163));
	dosetup->resize(QSize(fd+4, fd+4));

	ok=new eButton(this);
	ok->setText("[OK]");
	ok->move(QPoint(160, 200));
	ok->resize(QSize(90, fd+4));
	
	connect(ok, SIGNAL(selected()), SLOT(okPressed()));

	abort=new eButton(this);
	abort->setText("[ABORT]");
	abort->move(QPoint(270, 200));
	abort->resize(QSize(120, fd+4));

	connect(abort, SIGNAL(selected()), SLOT(abortPressed()));
	
}

eZapNetworkSetup::~eZapNetworkSetup()
{
}

void eZapNetworkSetup::fieldSelected(int *number)
{
	focusNext(0);
}


void eZapNetworkSetup::okPressed()
{
	int eIP[4], eMask[4], eDNS[4], eGateway[4];
	__u32 sip, snetmask, sdns, sgateway;
	for (int i=0; i<4; i++)
	{
		eIP[i]=ip->getNumber(i);
		eMask[i]=netmask->getNumber(i);
		eGateway[i]=gateway->getNumber(i);
		eDNS[i]=dns->getNumber(i);
	}
	pack(sip, eIP);
	pack(snetmask, eMask);
	pack(sdns, eDNS);
	pack(sgateway, eGateway);

	qDebug("IP: %d.%d.%d.%d, Netmask: %d.%d.%d.%d, gateway %d.%d.%d.%d, DNS: %d.%d.%d.%d", 
		eIP[0], eIP[1],  eIP[2], eIP[3],
		eMask[0], eMask[1],  eMask[2], eMask[3],
		eGateway[0], eGateway[1], eGateway[2], eGateway[3],
		eDNS[0], eDNS[1],  eDNS[2], eDNS[3]);

	int sdosetup=dosetup->isChecked();
	
	eDVB::getInstance()->config.setKey("/elitedvb/network/ip", sip);
	eDVB::getInstance()->config.setKey("/elitedvb/network/netmask", snetmask);
	eDVB::getInstance()->config.setKey("/elitedvb/network/dns", sdns);
	eDVB::getInstance()->config.setKey("/elitedvb/network/gateway", sgateway);
	eDVB::getInstance()->config.setKey("/elitedvb/network/dosetup", sdosetup);
	
	eDVB::getInstance()->configureNetwork();
	close(1);
}

void eZapNetworkSetup::abortPressed()
{
	close(0);
}

int eZapNetworkSetup::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_DOWN:
			focusNext(0);
			return 1;
			break;
		case eRCInput::RC_UP:
			focusNext(1);
			return 1;
			break;
		}
	}
	return 0;
}
