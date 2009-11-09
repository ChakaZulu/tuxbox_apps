/*
	$Id: network_setup.h,v 1.1 2009/11/09 13:05:09 dbt Exp $

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

	$Log: network_setup.h,v $
	Revision 1.1  2009/11/09 13:05:09  dbt
	menue cleanup:
	parentallock, movieplayer_menue and network-setup for it's own modules
		
*/

#ifndef __network_setup__
#define __network_setup__

#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>

#include <driver/framebuffer.h>

#include <system/setting_helpers.h>
#include <system/configure_network.h>

#include <string>


class CNetworkSetup : public CMenuTarget, CChangeObserver
{
	private:
		CFrameBuffer *frameBuffer;
		CIPChangeNotifier *MyIPChanger;
		CNetworkConfig  networkConfig;
						
		int x, y, width, height, hheight, mheight;

		int network_dhcp;
		int network_automatic_start;
		std::string network_address;
		std::string network_netmask;
		std::string network_broadcast;
		std::string network_nameserver;
		std::string network_gateway;

		int old_network_dhcp;
		int old_network_automatic_start;
		std::string old_network_address;
		std::string old_network_netmask;
		std::string old_network_broadcast;
		std::string old_network_nameserver;
		std::string old_network_gateway;

		void hide();
		void restoreNetworkSettings(bool show_message = false);
		void prepareSettings();

		int saveChangesDialog();

		bool checkForIP();
		bool settingsChanged();

	public:	
		enum NETWORK_DHCP_MODE
		{
			NETWORK_DHCP_OFF =  0, //static
			NETWORK_DHCP_ON  =  1,
		};

		enum NETWORK_START_MODE
		{
			NETWORK_AUTOSTART_OFF =  0,
			NETWORK_AUTOSTART_ON  =  1,
		};

		enum NETWORK_NTP_MODE
		{
			NETWORK_NTP_OFF =  0,
			NETWORK_NTP_ON  =  1,
		};

		CNetworkSetup();
		~CNetworkSetup();


		void showNetworkSetup();
		void applyNetworkSettings();
		void saveNetworkSettings(bool show_message = false);
		
		int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif
