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

struct apid_info
{
    char    name[50];
    int     ctag;
    bool    is_ac3;
    unsigned int    pid;
};

struct st_audio_info
{
	char        name[100];
	ushort      count_apids;
	apid_info   apids[max_num_apids];
	int         selected;
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
		// Std-Copy
		CSubService(const CSubService &ss)
		{
			startzeit=ss.startzeit;
			dauer=ss.dauer;
			onid_sid=ss.onid_sid;
			tsid=ss.tsid;
			subservice_name=ss.subservice_name;
		}
		// Der Operator zum sortieren
		bool operator < (const CSubService& c) const
		{
			return startzeit <= c.startzeit;
		}
		unsigned int    onid_sid;
		unsigned short  tsid;
		time_t          startzeit;
		unsigned        dauer;
		string          subservice_name;
};

typedef std::multiset <CSubService, std::less<CSubService> > CSubServiceListSorted;

class CSubChannel_Infos
{
	public:
		CSubChannel_Infos(void)
		{
			name= "";
			are_subchannels= false;
		}
		CSubChannel_Infos(const std::string &aname, bool _are_subchannels)
		{
			name= aname;
			are_subchannels= _are_subchannels;
		}
		// Std-Copy
		CSubChannel_Infos(const CSubChannel_Infos &ni)
		{
			name=ni.name;
			list=ni.list;
			selected=ni.selected;
			are_subchannels= ni.are_subchannels;
		}
		void clear()
		{
			name= "";
			list.clear();
			selected= 0;
		}
		void clear(char *channel_name)
		{
			clear();
			name= channel_name;
		}
		bool has_subChannels_for(const std::string &aname)
		{
			return ( (list.size()> 1) && (name==aname) );
		}
		string                      name;
		vector<class CSubService>	list;
		unsigned                    selected;
		bool                        are_subchannels;
};



class CRemoteControl
{
		st_rmsg             remotemsg;
		pthread_mutex_t     send_mutex;
		st_audio_info       audio_chans_int;
		CSubChannel_Infos   subChannels_internal;

		unsigned int    i_ecmpid;
		unsigned int    i_vpid;
		unsigned int    i_vtxtpid;

		void send();

		pthread_t       thrSender;
		pthread_cond_t  send_cond;


		static void * RemoteControlThread (void *arg);
		void getNVODs( char *channel_name );
		void getAPID_Names();

	public:
		st_audio_info       audio_chans;
		unsigned int        ecmpid;
		unsigned int        vpid;
		unsigned int        vtxtpid;

		CRemoteControl();
		void zapTo_onid_sid( unsigned int onid_sid, string channame );
		void queryAPIDs();
		void setAPID(int APID);
		string setSubChannel(unsigned numSub);
		string subChannelUp();
		string subChannelDown();
		void CopySubChannelsToZapit( const CSubChannel_Infos& subChannels );
		void shutdown();
		void radioMode();
		void tvMode();

		void CopyPIDs();
		const CSubChannel_Infos getSubChannels();
};


#endif



