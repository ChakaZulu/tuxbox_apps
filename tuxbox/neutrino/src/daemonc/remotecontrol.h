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


#include "sections/sectionsdMsg.h"
#include "zapitclient.h"

using namespace std;
#include <vector>
#include <set>
#include <string>

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

typedef std::vector<CSubService> CSubServiceListSorted;

class CRemoteControl
{
		int								current_programm_timer;
		unsigned long long				zap_completion_timeout;

		void getNVODs();
		void processAPIDnames();
		void getSubChannels();
		void copySubChannelsToZapit();

	public:
		unsigned int					current_onid_sid;
		unsigned int					current_sub_onid_sid;
		unsigned long long				current_EPGid;
		unsigned long long				next_EPGid;
		CZapitClient::responseGetPIDs	current_PIDs;

		// APID - Details
		bool							has_ac3;
		bool							has_unresolved_ctags;

		// SubChannel/NVOD - Details
		CSubServiceListSorted			subChannels;
		int								selected_subchannel;
		bool                        	are_subchannels;
		bool							needs_nvods;
		int								director_mode;

		// Video / Parental-Lock
		bool							is_video_started;
		unsigned int					zapCount;

		CRemoteControl();
		void zapTo_onid_sid( unsigned int onid_sid, string channame, bool start_video = true );
		void startvideo();
		void stopvideo();
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



