#include <netinet/in.h>
#include "setupnetwork.h"
#include "elabel.h"
#include "edvb.h"
#include "enumber.h"
#include "ebutton.h"
#include "echeckbox.h"
#include "rc.h"
#include "eskin.h"

#include <core/base/i18n.h>

#include <core/system/econfig.h>

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

eZapNetworkSetup::eZapNetworkSetup():
	eWindow(0)
{
	setText(_("Network setup"));
	cmove(ePoint(150, 136));
	cresize(eSize(420, 290));

	__u32 sip=ntohl(0x0a000061), snetmask=ntohl(0xFF000000), sdns=ntohl(0x7f000001), sgateway=ntohl(0x7f000001);
	int de[4];
	int sdosetup=0;
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eConfig::getInstance()->getKey("/elitedvb/network/ip", sip);
	eConfig::getInstance()->getKey("/elitedvb/network/netmask", snetmask);
	eConfig::getInstance()->getKey("/elitedvb/network/dns", sdns);
	eConfig::getInstance()->getKey("/elitedvb/network/gateway", sgateway);
	eConfig::getInstance()->getKey("/elitedvb/network/dosetup", sdosetup);

	eLabel *l=new eLabel(this);
	l->setText("IP:");
	l->move(ePoint(10, 0));
	l->resize(eSize(150, fd+4));

	unpack(sip, de);
	ip=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	ip->move(ePoint(160, 0));
	ip->resize(eSize(200, fd+4));
	ip->setFlags(eNumber::flagDrawPoints);

	l=new eLabel(this);
	l->setText("Netmask:");
	l->move(ePoint(10, 40));
	l->resize(eSize(150, fd+4));

	unpack(snetmask, de);
	netmask=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	netmask->move(ePoint(160, 40));
	netmask->resize(eSize(200, fd+4));
	netmask->setFlags(eNumber::flagDrawPoints);
	
	l=new eLabel(this);
	l->setText("Nameserver:");
	l->move(ePoint(10, 80));
	l->resize(eSize(150, fd+4));

	unpack(sdns, de);
	dns=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	dns->move(ePoint(160, 80));
	dns->resize(eSize(200, fd+4));
	dns->setFlags(eNumber::flagDrawPoints);

	l=new eLabel(this);
	l->setText("Gateway:");
	l->move(ePoint(10, 120));
	l->resize(eSize(150, fd+4));

	unpack(sgateway, de);
	gateway=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	gateway->move(ePoint(160, 120));
	gateway->resize(eSize(200, fd+4));
	gateway->setFlags(eNumber::flagDrawPoints);

	CONNECT(ip->selected, eZapNetworkSetup::fieldSelected);

	dosetup=new eCheckbox(this, sdosetup, fd);
	dosetup->setText("Configure Network");
	dosetup->move(ePoint(100, 163));
	dosetup->resize(eSize(fd+4+240, fd+4));

	ok=new eButton(this);
	ok->setText("[OK]");
	ok->move(ePoint(160, 200));
	ok->resize(eSize(90, fd+4));
	
	CONNECT(ok->selected, eZapNetworkSetup::okPressed);

	abort=new eButton(this);
	abort->setText("[ABORT]");
	abort->move(ePoint(270, 200));
	abort->resize(eSize(120, fd+4));

	CONNECT(abort->selected, eZapNetworkSetup::abortPressed);
}

eZapNetworkSetup::~eZapNetworkSetup()
{
}

void eZapNetworkSetup::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
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

	eDebug("IP: %d.%d.%d.%d, Netmask: %d.%d.%d.%d, gateway %d.%d.%d.%d, DNS: %d.%d.%d.%d",
		eIP[0], eIP[1],  eIP[2], eIP[3],
		eMask[0], eMask[1],  eMask[2], eMask[3],
		eGateway[0], eGateway[1], eGateway[2], eGateway[3],
		eDNS[0], eDNS[1],  eDNS[2], eDNS[3]);

	int sdosetup=dosetup->isChecked();
	
	eConfig::getInstance()->setKey("/elitedvb/network/ip", sip);
	eConfig::getInstance()->setKey("/elitedvb/network/netmask", snetmask);
	eConfig::getInstance()->setKey("/elitedvb/network/dns", sdns);
	eConfig::getInstance()->setKey("/elitedvb/network/gateway", sgateway);
	eConfig::getInstance()->setKey("/elitedvb/network/dosetup", sdosetup);
	
	eDVB::getInstance()->configureNetwork();
	close(1);
}

void eZapNetworkSetup::abortPressed()
{
	close(0);
}

