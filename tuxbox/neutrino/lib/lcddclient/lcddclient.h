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

#ifndef __lcddclient__
#define __lcddclient__

#include <string>

using namespace std;


class CLcddClient
{
		int sock_fd;

		bool lcdd_connect();
		bool send(char* data, int size);
		bool receive(char* data, int size);
		bool lcdd_close();

	public:
		static const std::string getSystemId();

		static const bool MUTE_ON  = true;
		static const bool MUTE_OFF = false;

		enum mode
		{
			MODE_TVRADIO,
			MODE_SCART,
			MODE_MENU,
			MODE_SAVER,
			MODE_SHUTDOWN,
			MODE_STANDBY
		};

		CLcddClient();

		void setMode(char mode, string head="");
		void setMenuText(char pos, string text, char highlight=0);
		void setServiceName(string name);
		void setMute(bool);
		void setVolume(char);
		void shutdown();

		void setBrightness(int brightness);
		int getBrightness();

		void setBrightnessStandby(int brightness);
		int getBrightnessStandby();

		void setContrast(int contrast);
		int getContrast();

		void setPower(bool power);
		bool getPower();

		void setInverse(bool inverse);
		bool getInverse();

		void update();			// applies new brightness, contrast, etc
		void pause();			// for plugins only
		void resume();			// for plugins only
};

#endif
