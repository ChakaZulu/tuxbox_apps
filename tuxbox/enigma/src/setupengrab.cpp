#include <setupengrab.h>
#include <plugin.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <lib/gui/ewindow.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/listbox.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/epgcache.h>
#include <lib/socket/socket.h>
#include <lib/base/estring.h>
#include <lib/gui/enumber.h>
#include <lib/gui/statusbar.h>

ENgrabSetup::ENgrabSetup():
	eWindow(0)
{
	setText(_("Ngrab Server"));
	cmove(ePoint(150, 136));
	cresize(eSize(390, 210));

	struct in_addr sinet_address;
	int nsrvport;
	int de[4];
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	if ( eConfig::getInstance()->getKey("/elitedvb/network/nserver", sinet_address.s_addr) )
		sinet_address.s_addr = 0xC0A80028; // 192.168.0.40
	if ( eConfig::getInstance()->getKey("/elitedvb/network/nservport", nsrvport ) )
		nsrvport = 4000;

	eLabel *l=new eLabel(this);
	l->setText("Srv IP:");
	l->move(ePoint(10, 20));
	l->resize(eSize(150, fd+4));

	eNumber::unpack(sinet_address.s_addr, de);
	inet_address=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	inet_address->move(ePoint(160, 20));
	inet_address->resize(eSize(200, fd+10));
	inet_address->setFlags(eNumber::flagDrawPoints);
	inet_address->setHelpText(_("enter IP Adress of the Ngrab Server (0..9, left, right)"));
	inet_address->loadDeco();

	l=new eLabel(this);
	l->setText("Srv Port:");
	l->move(ePoint(10, 60));
	l->resize(eSize(150, fd+4));

	srvport=new eNumber(this, 1, 0, 9999, 4, &nsrvport, 0, l);
	srvport->move(ePoint(160, 60));
	srvport->resize(eSize(200, fd+10));
	srvport->setFlags(eNumber::flagDrawPoints);
	srvport->setHelpText(_("enter ngrab server port (standard is 4000"));
	srvport->loadDeco();

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(10, 120));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	CONNECT(ok->selected, ENgrabSetup::okPressed);

	abort=new eButton(this);
	abort->loadDeco();
	abort->setText(_("abort"));
	abort->setShortcut("red");
	abort->setShortcutPixmap("red");
	abort->move(ePoint(200, 120));
	abort->resize(eSize(170, 40));
	abort->setHelpText(_("ignore changes and return"));
	CONNECT(abort->selected, ENgrabSetup::abortPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
}

ENgrabSetup::~ENgrabSetup()
{
}

void ENgrabSetup::okPressed()
{
	int einet_address[4];
	int nsrvport;

	struct in_addr sinet_address;

	for (int i=0; i<4; i++)
		einet_address[i] = inet_address->getNumber(i);

	eNumber::pack(sinet_address.s_addr, einet_address);

	nsrvport = srvport->getNumber();

	eDebug("write ip = %04x, port = %d", sinet_address.s_addr, nsrvport );
	eConfig::getInstance()->setKey("/elitedvb/network/nserver", sinet_address.s_addr );
	
	eConfig::getInstance()->setKey("/elitedvb/network/nservport", nsrvport);

	close(0);
}

void ENgrabSetup::abortPressed()
{
	close(0);
}
