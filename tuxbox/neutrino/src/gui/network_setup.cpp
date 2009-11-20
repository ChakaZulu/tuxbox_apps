/*
	$Id: network_setup.cpp,v 1.3 2009/11/20 22:44:19 dbt Exp $

	network setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Log: network_setup.cpp,v $
	Revision 1.3  2009/11/20 22:44:19  dbt
	reworked network_setup
	fix: netmask and broadcoast menue entries
	
	Revision 1.2  2009/11/09 20:21:55  dbt
	compiler warning removed
	
	Revision 1.1  2009/11/09 13:05:09  dbt
	menue cleanup:
	parentallock, movieplayer_menue and network-setup for it's own modules
		
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/network_setup.h"

#include "gui/nfs.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/hintbox.h>

#include <driver/screen_max.h>

#include <system/debug.h>



CNetworkSetup::CNetworkSetup()
{
	frameBuffer = CFrameBuffer::getInstance();
	networkConfig = CNetworkConfig::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	network_automatic_start = networkConfig->automatic_start;
	network_dhcp 		= networkConfig->inet_static ? NETWORK_DHCP_OFF : NETWORK_DHCP_ON;
	network_address		= networkConfig->address;
	network_netmask		= networkConfig->netmask;
	network_broadcast	= networkConfig->broadcast;
	network_nameserver	= networkConfig->nameserver;
	network_gateway		= networkConfig->gateway;

	old_network_automatic_start 	= networkConfig->automatic_start;
	old_network_dhcp 		= networkConfig->inet_static ? NETWORK_DHCP_OFF : NETWORK_DHCP_ON;
	old_network_address		= networkConfig->address;
	old_network_netmask		= networkConfig->netmask;
	old_network_broadcast		= networkConfig->broadcast;
	old_network_nameserver		= networkConfig->nameserver;
	old_network_gateway		= networkConfig->gateway;

}

CNetworkSetup::~CNetworkSetup()
{
	delete networkConfig;
}


int CNetworkSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}
	
	if(actionKey=="networkapply")
	{
		applyNetworkSettings();
		return res;
	}
	else if(actionKey=="networktest")
	{
		printf("[network setup] doing network test...\n");
		testNetworkSettings(	networkConfig->address.c_str(), 
					networkConfig->netmask.c_str(), 
					networkConfig->broadcast.c_str(), 
					networkConfig->gateway.c_str(), 
					networkConfig->nameserver.c_str(), 
					networkConfig->inet_static);
		return res;
	}
	else if(actionKey=="networkshow")
	{
		dprintf(DEBUG_INFO, "show current network settings...\n");
		showCurrentNetworkSettings();
		return res;
	}
	else if(actionKey=="networksave")
	{
		saveNetworkSettings();
		return res;
	}

	printf("[network setup] init network setup...\n");
	showNetworkSetup();
	
	return res;
}

void CNetworkSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width, height);
}

#define OPTIONS_NTPENABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_NTPENABLE_OPTIONS[OPTIONS_NTPENABLE_OPTION_COUNT] =
{
	{ CNetworkSetup::NETWORK_NTP_OFF, LOCALE_OPTIONS_NTP_OFF },
	{ CNetworkSetup::NETWORK_NTP_ON, LOCALE_OPTIONS_NTP_ON }
};



void CNetworkSetup::showNetworkSetup()
{
	bool loop = true;

	while (loop)
	{		
		//menue init
		CMenuWidget* networkSettings = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS, width);

		//subhead
		networkSettings->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MAINSETTINGS_NETWORK));

		//apply button
		CMenuForwarder *m0 = new CMenuForwarder(LOCALE_NETWORKMENU_SETUPNOW, true, NULL, this, "networkapply", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);
	
		//prepare input entries
		CIPInput * networkSettings_NetworkIP  = new CIPInput(LOCALE_NETWORKMENU_IPADDRESS , network_address   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, this);
		CIPInput * networkSettings_NetMask    = new CIPInput(LOCALE_NETWORKMENU_NETMASK   , network_netmask   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
		CIPInput * networkSettings_Broadcast  = new CIPInput(LOCALE_NETWORKMENU_BROADCAST , network_broadcast , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
		CIPInput * networkSettings_Gateway    = new CIPInput(LOCALE_NETWORKMENU_GATEWAY   , network_gateway   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
		CIPInput * networkSettings_NameServer = new CIPInput(LOCALE_NETWORKMENU_NAMESERVER, network_nameserver, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	

		//auto start
		CMenuOptionChooser* o1 = new CMenuOptionChooser(LOCALE_NETWORKMENU_SETUPONSTARTUP, &network_automatic_start, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

		//dhcp
		network_dhcp 	= networkConfig->inet_static ? NETWORK_DHCP_OFF : NETWORK_DHCP_ON;
	
		CMenuForwarder *m1 = new CMenuForwarder(LOCALE_NETWORKMENU_IPADDRESS , networkConfig->inet_static, network_address   , networkSettings_NetworkIP );
		CMenuForwarder *m2 = new CMenuForwarder(LOCALE_NETWORKMENU_NETMASK   , networkConfig->inet_static, network_netmask   , networkSettings_NetMask   );
		CMenuForwarder *m3 = new CMenuForwarder(LOCALE_NETWORKMENU_BROADCAST , networkConfig->inet_static, network_broadcast , networkSettings_Broadcast );
		CMenuForwarder *m4 = new CMenuForwarder(LOCALE_NETWORKMENU_GATEWAY   , networkConfig->inet_static, network_gateway   , networkSettings_Gateway   );
		CMenuForwarder *m5 = new CMenuForwarder(LOCALE_NETWORKMENU_NAMESERVER, networkConfig->inet_static, network_nameserver, networkSettings_NameServer);
		
		CDHCPNotifier* dhcpNotifier = new CDHCPNotifier(m1,m2,m3,m4,m5);
		CMenuOptionChooser* o2 = new CMenuOptionChooser(LOCALE_NETWORKMENU_DHCP, &network_dhcp, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, dhcpNotifier);		
		
		//paint menu items
		//intros
		networkSettings->addItem(GenericMenuSeparator);
		networkSettings->addItem(GenericMenuBack);
		networkSettings->addItem(GenericMenuSeparatorLine);
	
		networkSettings->addItem( m0 );
	
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_TEST, true, NULL, this, "networktest", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_SHOW, true, NULL, this, "networkshow", CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));
		networkSettings->addItem(GenericMenuSeparatorLine);
	
		networkSettings->addItem(o1);
		networkSettings->addItem(GenericMenuSeparatorLine);
		networkSettings->addItem(o2);
		networkSettings->addItem(GenericMenuSeparatorLine);
	
		networkSettings->addItem( m1);
		networkSettings->addItem( m2);
		networkSettings->addItem( m3);
	
		networkSettings->addItem(GenericMenuSeparatorLine);
		networkSettings->addItem( m4);
		networkSettings->addItem( m5);
	
		//ntp
		//prepare ntp input
		CSectionsdConfigNotifier* sectionsdConfigNotifier = new CSectionsdConfigNotifier;
		CStringInputSMS * networkSettings_NtpServer = new CStringInputSMS(LOCALE_NETWORKMENU_NTPSERVER, &g_settings.network_ntpserver, 30, LOCALE_NETWORKMENU_NTPSERVER_HINT1, LOCALE_NETWORKMENU_NTPSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-. ", sectionsdConfigNotifier);
	
		CStringInput * networkSettings_NtpRefresh = new CStringInput(LOCALE_NETWORKMENU_NTPREFRESH, &g_settings.network_ntprefresh, 3,LOCALE_NETWORKMENU_NTPREFRESH_HINT1, LOCALE_NETWORKMENU_NTPREFRESH_HINT2 , "0123456789 ", sectionsdConfigNotifier);
		networkSettings->addItem(GenericMenuSeparatorLine);
		CMenuWidget* ntp = new CMenuWidget(LOCALE_MAINSETTINGS_NETWORK, NEUTRINO_ICON_SETTINGS, width);
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_NTPTITLE, true, NULL, ntp, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
		ntp->addItem(new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_NETWORKMENU_NTPTITLE));
		ntp->addItem(GenericMenuSeparator);
		ntp->addItem(GenericMenuBack);
		ntp->addItem(GenericMenuSeparatorLine);
		CMenuOptionChooser *ntp1 = new CMenuOptionChooser(LOCALE_NETWORKMENU_NTPENABLE, &g_settings.network_ntpenable, OPTIONS_NTPENABLE_OPTIONS, OPTIONS_NTPENABLE_OPTION_COUNT, true, sectionsdConfigNotifier);
		CMenuForwarder *ntp2 = new CMenuForwarder( LOCALE_NETWORKMENU_NTPSERVER, true , g_settings.network_ntpserver, networkSettings_NtpServer );
		CMenuForwarder *ntp3 = new CMenuForwarder( LOCALE_NETWORKMENU_NTPREFRESH, true , g_settings.network_ntprefresh, networkSettings_NtpRefresh );
		
		ntp->addItem( ntp1);
		ntp->addItem( ntp2);
		ntp->addItem( ntp3);
	
	#ifdef ENABLE_GUI_MOUNT
		CMenuWidget* networkmounts = new CMenuWidget(LOCALE_MAINSETTINGS_NETWORK, NEUTRINO_ICON_SETTINGS, width);
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_MOUNT, true, NULL, networkmounts, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
		networkmounts->addItem(new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_NETWORKMENU_MOUNT));
		networkmounts->addItem(GenericMenuSeparator);
		networkmounts->addItem(GenericMenuBack);
		networkmounts->addItem(GenericMenuSeparatorLine);
		networkmounts->addItem(new CMenuForwarder(LOCALE_NFS_MOUNT , true, NULL, new CNFSMountGui(), NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
		networkmounts->addItem(new CMenuForwarder(LOCALE_NFS_UMOUNT, true, NULL, new CNFSUmountGui(), NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	#endif
		networkSettings->exec(NULL, "");
		networkSettings->hide();
		delete networkSettings;
					
		// Check for changes
 		loop = settingsChanged();
 	}

}

//returns true, if any settings were changed
bool CNetworkSetup::settingsChanged()
{
	bool ret = false;

	if (networkConfig->modified_from_orig())
	{
		//open message box
		ret =  saveChangesDialog();
	}
	else
	{	
		ret = false;
	}

	return ret;
}

//prepares internal settings before commit
void CNetworkSetup::prepareSettings()
{
	networkConfig->automatic_start 	= network_automatic_start;
	networkConfig->inet_static 	= (network_dhcp ? false : true);
	networkConfig->address 		= network_address;
	networkConfig->netmask 		= network_netmask;
	networkConfig->broadcast 	= network_broadcast;
	networkConfig->gateway 		= network_gateway;
	networkConfig->nameserver 	= network_nameserver;
}

//check for ip-address, if dhcp disabled, returns false if no address definied and show message
bool CNetworkSetup::checkForIP()
{
	bool ret = true;

	if (!network_dhcp && network_address.empty()) //no ip definied
	{
		ShowMsgUTF(LOCALE_MAINSETTINGS_NETWORK, g_Locale->getText(LOCALE_NETWORKMENU_ERROR_NO_ADDRESS), CMessageBox::mbrBack, 
				CMessageBox::mbBack, 
				NEUTRINO_ICON_ERROR, 
				width);
		ret = false;
	}

	return ret;
}

//saves settings without apply, reboot is required 
void CNetworkSetup::saveNetworkSettings(bool show_message)
{
	bool show_msg = show_message;

	printf("[network setup] saving current network settings...\n");

	prepareSettings();

  	networkConfig->commitConfig();

	if (show_msg)
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_NETWORKMENU_SETUPSAVED), width , g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR], NEUTRINO_ICON_INFO); // UTF-8

}

//saves settings and apply, reboot not required
void CNetworkSetup::applyNetworkSettings()
{
	printf("[network setup] apply network settings...\n");

	CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_NETWORKMENU_APPLY_SETTINGS)); // UTF-8
	hintBox->paint();

	prepareSettings();

	networkConfig->stopNetwork();
	networkConfig->commitConfig();
	networkConfig->startNetwork();

	hintBox->hide();
	delete hintBox;
}

//open a message dialog with three buttons,
//yes:		applies networksettings and exit network setup
//no:		saves networksettings and exit network setup
//back: 	exit message dialog, goes back to network setup 
int CNetworkSetup::saveChangesDialog()
{
	if (!checkForIP())
		return 1;	

	// Save the settings after changes, if user wants to!
	int result = 	ShowMsgUTF(LOCALE_MAINSETTINGS_NETWORK, g_Locale->getText(LOCALE_NETWORKMENU_APPLY_SETTINGS_NOW), CMessageBox::mbrYes, 
			CMessageBox::mbYes | 
			CMessageBox::mbNo | 
			CMessageBox::mbBack, 
			NEUTRINO_ICON_QUESTION, 
			width);
	
	switch(result)
	{
		case 0: //yes
			applyNetworkSettings();
			return 0;
			break;
	
		case 1: //no
			saveNetworkSettings(true);
			return 0;
			break;
	
		default://back
			restoreNetworkSettings(true);
			return 1;
			break;
	}
}

//restores settings without any changes if user wants to
void CNetworkSetup::restoreNetworkSettings(bool show_message)
{
	bool show_msg = show_message;
	int result = 1;

	if (show_msg)
	{
		result = 	ShowMsgUTF(LOCALE_MAINSETTINGS_NETWORK, g_Locale->getText(LOCALE_NETWORKMENU_RESET_SETTINGS_NOW), CMessageBox::mbrNo, 
				CMessageBox::mbYes | 
				CMessageBox::mbNo , 
				NEUTRINO_ICON_QUESTION, 
				width);
	}

	if (result == 0) //yes
	{
		network_automatic_start	= old_network_automatic_start;
		network_dhcp		= old_network_dhcp;
		network_address		= old_network_address;
		network_netmask		= old_network_netmask;
		network_broadcast	= old_network_broadcast;
		network_nameserver	= old_network_nameserver;
		network_gateway		= old_network_gateway;

		networkConfig->automatic_start 	= network_automatic_start;
		networkConfig->inet_static 	= (network_dhcp ? false : true);
		networkConfig->address 		= network_address;
		networkConfig->netmask 		= network_netmask;
		networkConfig->broadcast 	= network_broadcast;
		networkConfig->gateway 		= network_gateway;
		networkConfig->nameserver 	= network_nameserver;

		networkConfig->commitConfig();
	}

}

bool CNetworkSetup::changeNotify(const neutrino_locale_t, void * Data)
{
	char ip[16];
	unsigned char _ip[4];
	sscanf((char*) Data, "%hhu.%hhu.%hhu.%hhu", &_ip[0], &_ip[1], &_ip[2], &_ip[3]);

	sprintf(ip, "%hhu.%hhu.%hhu.255", _ip[0], _ip[1], _ip[2]);
	networkConfig->broadcast = ip;
	network_broadcast = networkConfig->broadcast;

	networkConfig->netmask = (_ip[0] == 10) ? "255.0.0.0" : "255.255.255.0";
	network_netmask = networkConfig->netmask;
	return true;
}
