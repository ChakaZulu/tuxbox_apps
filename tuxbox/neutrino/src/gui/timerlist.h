/*
	Neutrino-GUI  -   DBoxII-Project

	Timerliste by Zwen
	
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

#ifndef __timerlist__
#define __timerlist__

#include "driver/framebuffer.h"
#include "gui/widget/menue.h"
#include "timerdclient.h"

using namespace std;



class CTimerList : public CMenuTarget
{
	public:

	private:
		CFrameBuffer		*frameBuffer;
		unsigned int		selected;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		int					fheight; // Fonthoehe Timerlist-Inhalt
		int					theight; // Fonthoehe Timerlist-Titel
		int               buttonHeight;

		CTimerdClient *Timer;
		CTimerd::TimerList timerlist;             // List of timers		
		CZapitClient::BouquetChannelList channellist;     

		int 			width;
		int 			height;
		int 			x;
		int 			y;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();

	public:
		CTimerList();
		~CTimerList();
		void updateEvents(void);
		int  show();
		int  exec(CMenuTarget* parent, string actionKey);
		string convertTimerType2String(CTimerEvent::CTimerEventTypes type);
		string convertTimerRepeat2String(CTimerEvent::CTimerEventRepeat rep);
};


#endif


