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

#ifndef __epgdata__
#define __epgdata__


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include "../driver/framebuffer.h"
#include "../driver/rcinput.h"
#include "../driver/fontrenderer.h"
#include "../widget/color.h"
#include "../helpers/settings.h"
#include "../options.h"

#include "sectionsdMsg.h"

#include "pthread.h"

#include <string>
#include <vector>

using namespace std;


#define SA struct sockaddr
#define SAI struct sockaddr_in



struct EPGData
{
  char title[100];
  char info1[600];
  char info2[3000];
  char date[20];
  char start[10];
  char end[10];
  char done[6];
};


class CEpgData
{
	private:
		EPGData			epgData;

        unsigned long long prev_id;
        time_t prev_zeit;
        unsigned long long next_id;
        time_t next_zeit;

        unsigned long long current_id;
        time_t current_zeit;

		int			ox, oy, sx, sy;
		int			emptyLineCount, info1_lines;
        int         textCount;
		vector<string>		epgText;
		int			topheight,topboxheight;
		int			botheight,botboxheight;
		int			medlineheight,medlinecount;

		void GetEPGData( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit );
        void GetPrevNextEPGData( unsigned long long id, time_t* startzeit );
		void addTextToArray( string text );
		void processTextToArray( char* text );
		void showText( int startPos, int ypos );

	public:

		CEpgData();
		void start( );
		void show( string channelName, unsigned int onid_tsid, unsigned long long id = 0, time_t* startzeit = NULL, bool doLoop = true);
		void hide();
};


#endif

