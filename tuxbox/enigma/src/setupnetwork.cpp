#include <setupnetwork.h>

#include <netinet/in.h>

#include <lib/base/i18n.h>

#include <lib/dvb/edvb.h>
#include <lib/gui/elabel.h>
#include <lib/gui/enumber.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/driver/rc.h>
#include <lib/system/econfig.h>

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
	cresize(eSize(390, 300));

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
	l->move(ePoint(10, 20));
	l->resize(eSize(150, fd+4));

	unpack(sip, de);
	ip=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	ip->move(ePoint(160, 20));
	ip->resize(eSize(200, fd+10));
	ip->setFlags(eNumber::flagDrawPoints);
	ip->setHelpText(_("enter IP Adress of the box (0..9, left, right)"));
	ip->loadDeco();
	CONNECT(ip->selected, eZapNetworkSetup::fieldSelected);

	l=new eLabel(this);
	l->setText("Netmask:");
	l->move(ePoint(10, 60));
	l->resize(eSize(150, fd+4));

	unpack(snetmask, de);
	netmask=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	netmask->move(ePoint(160, 60));
	netmask->resize(eSize(200, fd+10));
	netmask->setFlags(eNumber::flagDrawPoints);
	netmask->setHelpText(_("enter netmask of your network (0..9, left, right)"));
	netmask->loadDeco();
	CONNECT(netmask->selected, eZapNetworkSetup::fieldSelected);
	
	l=new eLabel(this);
	l->setText("Nameserver:");
	l->move(ePoint(10, 100));
	l->resize(eSize(150, fd+4));

	unpack(sdns, de);
	dns=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	dns->move(ePoint(160, 100));
	dns->resize(eSize(200, fd+10));
	dns->setFlags(eNumber::flagDrawPoints);
	dns->setHelpText(_("enter your domain name server (0..9, left, right)"));
	dns->loadDeco();
	CONNECT(dns->selected, eZapNetworkSetup::fieldSelected);

	l=new eLabel(this);
	l->setText("Gateway:");
	l->move(ePoint(10, 140));
	l->resize(eSize(150, fd+4));

	unpack(sgateway, de);
	gateway=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	gateway->move(ePoint(160, 140));
	gateway->resize(eSize(200, fd+10));
	gateway->setFlags(eNumber::flagDrawPoints);
	gateway->setHelpText(_("enter ip of your gateway (0..9, left, right)"));
	gateway->loadDeco();
	CONNECT(gateway->selected, eZapNetworkSetup::fieldSelected);

	dosetup=new eCheckbox(this, sdosetup, 1);
	dosetup->setText("Configure Network");
	dosetup->move(ePoint(100, 183));
	dosetup->resize(eSize(fd+4+240, fd+4));
	dosetup->setHelpText(_("enable/disable network config (ok)"));

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(10, 220));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("close window and save changes"));
	ok->loadDeco();
	
	CONNECT(ok->selected, eZapNetworkSetup::okPressed);

	abort=new eButton(this);
	abort->loadDeco();
	abort->setText(_("abort"));
	abort->move(ePoint(200, 220));
	abort->resize(eSize(170, 40));
	abort->setHelpText(_("close window (no changes are saved)"));

	CONNECT(abort->selected, eZapNetworkSetup::abortPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
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
	eConfig::getInstance()->flush();
	
	eDVB::getInstance()->configureNetwork();
	close(1);
}

void eZapNetworkSetup::abortPressed()
{
	close(0);
}

