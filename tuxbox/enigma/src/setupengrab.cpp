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

	struct in_addr sinet_address;
	int nsrvport;
	int de[4];

	if ( eConfig::getInstance()->getKey("/elitedvb/network/nserver", sinet_address.s_addr) )
		sinet_address.s_addr = 0xC0A80028; // 192.168.0.40
	if ( eConfig::getInstance()->getKey("/elitedvb/network/nservport", nsrvport ) )
		nsrvport = 4000;


	eNumber::unpack(sinet_address.s_addr, de);
	inet_address=CreateSkinnedNumberWithLabel("inet_address",0, 4, 0, 255, 3, de, 0, "lsrvip");
	inet_address->setFlags(eNumber::flagDrawPoints);

	srvport=CreateSkinnedNumberWithLabel("srvport",0, 1, 0, 9999, 4, &nsrvport, 0, "lsrvport");
	srvport->setFlags(eNumber::flagDrawPoints);

	serverMAC=CreateSkinnedTextInputField("serverMAC",0,0,"lsrvmac");
	serverMAC->setUseableChars("01234567890abcdefABCDEF:");
	serverMAC->setMaxChars(17);

	char* sMAC=0;
	if ( eConfig::getInstance()->getKey("/elitedvb/network/hwaddress", sMAC ) )
		serverMAC->setText("00:00:00:00:00:00");
	else
	{
		serverMAC->setText(sMAC);
		free(sMAC);
	}

	CONNECT(CreateSkinnedButton("bServerMAC")->selected, ENgrabSetup::detectMAC );

	CONNECT(CreateSkinnedButton("ok")->selected, ENgrabSetup::okPressed);

	BuildSkin("ENgrabSetup");
	
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
		hide();
		eMessageBox::ShowBox(
			_("Please check your NGrab Server or the IP"),
			_("HW Address(MAC) detection failed"),
			eMessageBox::btOK|eMessageBox::iconInfo );
		show();
	}
}

#endif // DISABLE_NETWORK
