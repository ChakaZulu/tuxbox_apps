/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
*/

#ifndef __remotecontrol__
#define __remotecontrol__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include <string>

#include "pthread.h"
#include "semaphore.h"
#include <sys/wait.h>
#include <signal.h>
#include "zapit/getservices.h"
#include "sections/sectionsdMsg.h"
#include "zapitclient.h"

using namespace std;
#include <vector>
#include <set>

#define SA struct sockaddr
#define SAI struct sockaddr_in

struct st_rmsg
{
	unsigned char version;
	unsigned char cmd;
	unsigned char param;
	unsigned short param2;
	char param3[30];
};

class CSubService
{
	public:
		CSubService(const unsigned int &aonid_sid, const unsigned short &atsid, const string &asubservice_name)
		{
			onid_sid= aonid_sid;
			tsid= atsid;
			startzeit=0;
			dauer=0;
			subservice_name= asubservice_name;
		}
		CSubService(const unsigned int &aonid_sid, const unsigned short &atsid, const time_t &astartzeit, const unsigned adauer)
		{
			onid_sid= aonid_sid;
			tsid= atsid;
			startzeit=astartzeit;
			dauer=adauer;
			subservice_name= "";
		}

		unsigned int    onid_sid;
		unsigned short  tsid;
		time_t          startzeit;
		unsigned        dauer;
		string          subservice_name;
};

//typedef std::multiset <CSubService, std::less<CSubService> > CSubServiceListSorted;
typedef std::vector<CSubService> CSubServiceListSorted;

class CRemoteControl
{
		bool							waiting_for_zap_completion;

		void getNVODs();
		void processAPIDnames();
		void getSubChannels();
		void copySubChannelsToZapit();

	public:
		unsigned int					current_onid_sid;
		unsigned int					current_sub_onid_sid;
		unsigned long long				current_EPGid;
		CZapitClient::responseGetPIDs	current_PIDs;

		// APID - Details
		int								selected_apid;
		bool							has_ac3;
		bool							has_unresolved_ctags;

		// SubChannel/NVOD - Details
		CSubServiceListSorted			subChannels;
		int								selected_subchannel;
		bool                        	are_subchannels;
		bool							needs_nvods;

		CRemoteControl();
		void zapTo_onid_sid( unsigned int onid_sid, string channame );
		void queryAPIDs();
		void setAPID(int APID);
		string setSubChannel(unsigned numSub, bool force_zap = false );
		string subChannelUp();
		string subChannelDown();

		void radioMode();
		void tvMode();

		int handleMsg(uint msg, uint data);
};


#endif



