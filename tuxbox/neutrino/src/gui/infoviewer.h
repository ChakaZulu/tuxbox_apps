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
	private:
		CFrameBuffer	*frameBuffer;

		bool	gotTime;
		bool	recordModeActive;

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

		string						CurrentChannel;
		sectionsd::CurrentNextInfo	info_CurrentNext;
        unsigned int				current_onid_sid;

		char aspectRatio;

		int	 	sec_timer_id;
		int 	fadeTimer;

		void show_Data( bool calledFromEvent = false );
		void paintTime( bool show_dot, bool firstPaint );


		void showButton_Audio();
		void showButton_SubServices();

		void showIcon_16_9();
		void showIcon_VTXT();
		void showRecordIcon( bool show );

		void showFailure();
	public:

		bool	is_visible;

		CInfoViewer();

		void start();

		void showTitle( int ChanNum, string Channel, unsigned int onid_sid = 0, bool calledFromNumZap = false );
		void killTitle();
		sectionsd::CurrentNextInfo getEPG( unsigned int onid_sid );

		int handleMsg(uint msg, uint data);
};


#endif
