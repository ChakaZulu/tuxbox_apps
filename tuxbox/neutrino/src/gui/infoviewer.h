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

#ifndef __infoview__
#define __infoview__

#include "driver/rcinput.h"
#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "widget/color.h"
#include "helpers/settings.h"
#include "streaminfo.h"

#include "pthread.h"
#include "semaphore.h"
#include <sys/wait.h>
#include <signal.h>

#include "sections/sectionsdMsg.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/timeb.h>
#include <time.h>

#include <string>
#include <controldclient.h>

using namespace std;


#define SA struct sockaddr
#define SAI struct sockaddr_in


class CInfoViewer
{
		struct SubService
		{
			unsigned short  transportStreamId;
			unsigned short  originalNetworkId;
			unsigned short  serviceId;
			string          name;
		};

	private:
		bool		KillShowEPG;
		bool		gotTime;

		pthread_t	thrViewer;
		pthread_cond_t	epg_cond;
		pthread_mutex_t	epg_mutex;

		pthread_t	thrLangViewer;

		int		InfoHeightY;
		int		InfoHeightY_Info;
		bool	showButtonBar;
		int		BoxEndX;
		int		BoxEndY;
		int		BoxStartX;
		int		BoxStartY;
		int		ButtonWidth;

		int		ChanWidth;
		int		ChanHeight;
		int		ChanInfoX;

		string		CurrentChannel;
		unsigned int	Current_onid_tsid;
		char		*EPG_NotFound_Text;

		char running[50];
		char next[50];
		char runningStart[10];
		char nextStart[10];
		char runningDuration[10];
		char runningRest[20];
		char nextDuration[10];
		char runningPercent;
		unsigned char       Flag;

		char aspectRatio;

		static void * InfoViewerThread (void *arg);
		static void * LangViewerThread (void *arg);
		bool getEPGData( string channelName, unsigned int onid_tsid );

		void showData();
		void showWarte();
		void showButtonAudio();
		void showButtonNVOD( bool CalledFromShowData = false );
		void show16_9( bool showAnyWay = false );
	public:

		bool			is_visible;
		pthread_cond_t		cond_PIDs_available;
		vector<SubService*>	SubServiceList;

		CInfoViewer();

		void start();

		void showTitle( int ChanNum, string Channel, unsigned int onid_tsid = 0, bool CalledFromNumZap = false );
		void killTitle();

		const std::string getActiveChannelID();

		int handleMsg(uint msg, uint data);
};


#endif
