#include <setupnetwork.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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

static void unpack(const struct in_addr &l, int *t)
{
	uint8_t *tc = (uint8_t *) &l.s_addr;
	for (int i = 0; i < 4; ++i)
		t[i] = tc[i];
}

static void pack(struct in_addr &l, const int *const t)
{
	uint8_t *tc = (uint8_t *) &l.s_addr;
	for (int i = 0; i < 4; ++i)
		tc[i] = t[i];
}

// convert a netmask (255.255.255.0) into the length (24)
static int inet_ntom (const void *src, int *dst)
{
	struct in_addr *addr = (struct in_addr*) src;
	in_addr_t mask, num;

	mask = ntohl(addr->s_addr);

	for (num = mask; num & 1; num >>= 1);

	if (num != 0 && mask != 0)
	{
		for (num = ~mask; num & 1; num >>= 1);
		if (num)
			return 0;
	}

	for (num = 0; mask; mask <<= 1)
		num++;

	*dst = num;

	return 1;
}

// convert a length (24) into the netmask (255.255.255.0)
static int inet_mton (int src, void *dst)
{
	struct in_addr *addr = (struct in_addr*) dst;
	in_addr_t mask = 0;

	for(; src; src--)
		mask |= 1 << (32 - src);

	addr->s_addr = htonl(mask);

	return 1;
}

eZapNetworkSetup::eZapNetworkSetup():
	eWindow(0)
{
	setText(_("Network setup"));
	cmove(ePoint(150, 136));
	cresize(eSize(390, 300));

	int type = 0;
	struct in_addr sinet_address, sinet_netmask_addr, snameserver, sinet_gateway;
	inet_pton(AF_INET, "10.0.0.42", &sinet_address);
	int sinet_netmask = 24;
	inet_pton(AF_INET, "10.0.0.1", &snameserver);
	inet_pton(AF_INET, "10.0.0.1", &sinet_gateway);

	int de[4];
	int sdosetup=0;
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eConfig::getInstance()->getKey("/elitedvb/network/type", type);
	eConfig::getInstance()->getKey("/elitedvb/network/inet/address", sinet_address.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/inet/netmask", sinet_netmask);
	eConfig::getInstance()->getKey("/elitedvb/network/inet/gateway", sinet_gateway.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/nameserver", snameserver.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/dosetup", sdosetup);

	inet_mton(sinet_netmask, &sinet_netmask_addr);

	eLabel *l=new eLabel(this);
	l->setText("IP:");
	l->move(ePoint(10, 20));
	l->resize(eSize(150, fd+4));

	unpack(sinet_address, de);
	inet_address=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	inet_address->move(ePoint(160, 20));
	inet_address->resize(eSize(200, fd+10));
	inet_address->setFlags(eNumber::flagDrawPoints);
	inet_address->setHelpText(_("enter IP Adress of the box (0..9, left, right)"));
	inet_address->loadDeco();
	CONNECT(inet_address->selected, eZapNetworkSetup::fieldSelected);

	l=new eLabel(this);
	l->setText("Netmask:");
	l->move(ePoint(10, 60));
	l->resize(eSize(150, fd+4));

	unpack(sinet_netmask_addr, de);
	inet_netmask=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	inet_netmask->move(ePoint(160, 60));
	inet_netmask->resize(eSize(200, fd+10));
	inet_netmask->setFlags(eNumber::flagDrawPoints);
	inet_netmask->setHelpText(_("enter netmask of your network (0..9, left, right)"));
	inet_netmask->loadDeco();
	CONNECT(inet_netmask->selected, eZapNetworkSetup::fieldSelected);
	
	l=new eLabel(this);
	l->setText("Nameserver:");
	l->move(ePoint(10, 100));
	l->resize(eSize(150, fd+4));

	unpack(snameserver, de);
	nameserver=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	nameserver->move(ePoint(160, 100));
	nameserver->resize(eSize(200, fd+10));
	nameserver->setFlags(eNumber::flagDrawPoints);
	nameserver->setHelpText(_("enter your domain name server (0..9, left, right)"));
	nameserver->loadDeco();
	CONNECT(nameserver->selected, eZapNetworkSetup::fieldSelected);

	l=new eLabel(this);
	l->setText("Gateway:");
	l->move(ePoint(10, 140));
	l->resize(eSize(150, fd+4));

	unpack(sinet_gateway, de);
	inet_gateway=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	inet_gateway->move(ePoint(160, 140));
	inet_gateway->resize(eSize(200, fd+10));
	inet_gateway->setFlags(eNumber::flagDrawPoints);
	inet_gateway->setHelpText(_("enter ip of your gateway (0..9, left, right)"));
	inet_gateway->loadDeco();
	CONNECT(inet_gateway->selected, eZapNetworkSetup::fieldSelected);

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
	int type = 0;
	int einet_address[4], einet_netmask_addr[4], enameserver[4], einet_gateway[4];
	struct in_addr sinet_address, sinet_netmask_addr, snameserver, sinet_gateway;
	int sinet_netmask;
	for (int i=0; i<4; i++)
	{
		einet_address[i]	= inet_address->getNumber(i);
		einet_netmask_addr[i]	= inet_netmask->getNumber(i);
		enameserver[i]		= nameserver->getNumber(i);
		einet_gateway[i]	= inet_gateway->getNumber(i);
	}
	pack(sinet_address, einet_address);
	pack(sinet_netmask_addr, einet_netmask_addr);
	pack(snameserver, enameserver);
	pack(sinet_gateway, einet_gateway);
	inet_ntom(&sinet_netmask_addr, &sinet_netmask);

	eDebug("IP: %d.%d.%d.%d, Netmask: %d.%d.%d.%d, gateway %d.%d.%d.%d, DNS: %d.%d.%d.%d",
		einet_address[0], einet_address[1],  einet_address[2], einet_address[3],
		einet_netmask_addr[0], einet_netmask_addr[1],  einet_netmask_addr[2], einet_netmask_addr[3],
		einet_gateway[0], einet_gateway[1], einet_gateway[2], einet_gateway[3],
		enameserver[0], enameserver[1],  enameserver[2], enameserver[3]);

	int sdosetup=dosetup->isChecked();
	
	eConfig::getInstance()->setKey("/elitedvb/network/type", type);
	eConfig::getInstance()->setKey("/elitedvb/network/inet/address", sinet_address.s_addr);
	eConfig::getInstance()->setKey("/elitedvb/network/inet/netmask", sinet_netmask);
	eConfig::getInstance()->setKey("/elitedvb/network/inet/gateway", sinet_gateway.s_addr);
	eConfig::getInstance()->setKey("/elitedvb/network/nameserver", snameserver.s_addr);
	eConfig::getInstance()->setKey("/elitedvb/network/dosetup", sdosetup);
	eConfig::getInstance()->flush();
	
	eDVB::getInstance()->configureNetwork();
	close(1);
}

void eZapNetworkSetup::abortPressed()
{
	close(0);
}

