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

#ifndef __channellist__
#define __channellist__

#include "driver/framebuffer.h"
#include "system/lastchannel.h"
#include "gui/widget/menue.h"

#include <string>
#include <vector>

using namespace std;



class CChannelList
{
	public:
		class CChannel
		{
			private:
				unsigned long long	last_unlocked_EPGid;

			public:
				int         	key;
				int         	number;
				string      	name;
				unsigned int 	onid_sid;
				CChannelEvent	currentEvent;

				// flag that tells if channel is staticly locked by bouquet-locking
				bool bAlwaysLocked;

				// constructor
				CChannel();

				friend class CChannelList;
		};

	private:
		CFrameBuffer		*frameBuffer;
		unsigned int		selected;
		unsigned int		tuned;
		CLastChannel		lastChList;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int					fheight; // Fonthoehe Channellist-Inhalt
		int					theight; // Fonthoehe Channellist-Titel

		string				name;
		vector<CChannel*>	chanlist;
		CZapProtection* 	zapProtection;

		int 			width;
		int 			height;
		int 			x;
		int 			y;

		void paintDetails(int index);
		void clearItem2DetailsLine ();
		void paintItem2DetailsLine (int pos, int ch_index);
		void paintItem(int pos);
		void paint();
		void paintHead();
		void hide();

	public:
		CChannelList( const std::string& Name="" );
		~CChannelList();
		void addChannel(int key, int number, const std::string& name, unsigned int ids = 0);
		void addChannel(CChannel* chan);
		CChannel* getChannel( int number);
		CChannel* operator[]( uint index) { if (chanlist.size() > index) return chanlist[index]; else return NULL;};
		void setName(const std::string& Name);
		int getKey(int);
		string getActiveChannelName();
		int getActiveChannelNumber();
		unsigned int CChannelList::getActiveChannelOnid_sid();
		const string getActiveChannelID();
		CChannel* getChannelFromOnidSid(unsigned int onidSid);
		void zapTo(int pos);
		bool zapToOnidSid(unsigned int onid_sid);
		bool adjustToChannelID(const t_channel_id channel_id);
		bool showInfo(int pos);
		void updateEvents(void);
		int numericZap(int key);
		int  show();
		int	 exec();
		void quickZap(int key);
		int  hasChannel(int nChannelNr);
		void setSelected( int nChannelNr); // for adjusting bouquet's channel list after numzap or quickzap

		int handleMsg(uint msg, uint data);
};


#endif


