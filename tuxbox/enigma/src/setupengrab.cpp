#ifndef DISABLE_NETWORK

#include <setupengrab.h>
#include <plugin.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <lib/gui/ewindow.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/listbox.h>
#include <lib/gui/textinput.h>
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
	init_ENgrabSetup();
}
void ENgrabSetup::init_ENgrabSetup()
{
	setText(_("Ngrab Server"));
	cmove(ePoint(170, 146));
	cresize(eSize(390, 310));

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
	l->move(ePoint(20, 20));
	l->resize(eSize(140, fd+4));

	eNumber::unpack(sinet_address.s_addr, de);
	inet_address=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	inet_address->move(ePoint(160, 20));
	inet_address->resize(eSize(200, fd+10));
	inet_address->setFlags(eNumber::flagDrawPoints);
	inet_address->setHelpText(_("enter IP Adress of the Ngrab Server (0..9, left, right)"));
	inet_address->loadDeco();

	l=new eLabel(this);
	l->setText("Srv Port:");
	l->move(ePoint(20, 60));
	l->resize(eSize(140, fd+4));

	srvport=new eNumber(this, 1, 0, 9999, 4, &nsrvport, 0, l);
	srvport->move(ePoint(160, 60));
	srvport->resize(eSize(200, fd+10));
	srvport->setFlags(eNumber::flagDrawPoints);
	srvport->setHelpText(_("enter ngrab server port (standard is 4000)"));
	srvport->loadDeco();

	l=new eLabel(this);
	l->setText("Srv MAC:");
	l->move(ePoint(20,100));
	l->resize(eSize(140, fd+4));

	serverMAC=new eTextInputField(this);
	serverMAC->move(ePoint(160,100));
	serverMAC->resize(eSize(200, fd+10));
	serverMAC->setHelpText(_("enter MAC address of server (for wake on lan)"));
	serverMAC->setUseableChars("01234567890abcdefABCDEF:");
	serverMAC->setMaxChars(17);
	serverMAC->loadDeco();

	char* sMAC=0;
	if ( eConfig::getInstance()->getKey("/elitedvb/network/hwaddress", sMAC ) )
		serverMAC->setText("00:00:00:00:00:00");
	else
	{
		serverMAC->setText(sMAC);
		free(sMAC);
	}

	bServerMAC=new eButton(this);
	bServerMAC->move(ePoint(20,150));
	bServerMAC->resize(eSize(340,40));
	bServerMAC->setShortcut("blue");
	bServerMAC->setShortcutPixmap("blue");
	bServerMAC->setText(_("detect MAC Adress"));
	bServerMAC->setHelpText(_("try to autodetect server MAC address"));
	bServerMAC->loadDeco();
	CONNECT( bServerMAC->selected, ENgrabSetup::detectMAC );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 210));
	ok->resize(eSize(220, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	CONNECT(ok->selected, ENgrabSetup::okPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();
	
	setHelpID(91);
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
	eConfig::getInstance()->setKey("/elitedvb/network/hwaddress", serverMAC->getText().c_str() );
	eConfig::getInstance()->flush();

	close(0);
}

void ENgrabSetup::detectMAC()
{
	eString serverip;

	serverip.sprintf("%d.%d.%d.%d",
		inet_address->getNumber(0),
		inet_address->getNumber(1),
		inet_address->getNumber(2),
		inet_address->getNumber(3) );

	if ( system(eString().sprintf("ping -c 2 %s",serverip.c_str()).c_str()) == 0 )
	{
		FILE *f = fopen("/proc/net/arp", "r");
		if ( f )
		{
			char line[1024];
			fgets(line, 1024, f);
			int HWAddrPos = strstr(line, "HW address") - line;
			if ( HWAddrPos  <  0)
			{
				fclose(f);
				return;
			}
			while (1)
			{
				if (!fgets(line, 1024, f))
					break;
				if ( strstr(line, serverip.c_str() ) )
				{
					serverMAC->setText( eString(line+HWAddrPos,17) );
					break;
				}       
			}
			fclose(f);
		}
	}
	else
	{
		eMessageBox mb(
			_("Please check your NGrab Server or the IP"),
			_("HW Address(MAC) detection failed"),
			eMessageBox::btOK|eMessageBox::iconInfo );
		hide();
		mb.show();
		mb.exec();
		mb.hide();
		show();
	}
}

#endif // DISABLE_NETWORK
