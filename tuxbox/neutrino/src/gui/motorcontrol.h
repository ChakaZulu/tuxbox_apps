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


#ifndef __motorcontrol__
#define __motorcontrol__

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>

#include <string>


#define STEP_MODE_OFF 0
#define STEP_MODE_ON 1 
#define STEP_MODE_TIMED 2       

class CMotorControl : public CMenuTarget
{
	private:
		CFrameBuffer	*frameBuffer;
		int x;
		int y;
		int width;
		int height;
		int hheight,mheight,sheight; // head/menu font height
		int head_height;
		int menue_height;
		int status_height;
		int ypos;
		int ypos_menue;
		
		int8_t stepSize;
		int32_t stepDelay;
		int stepMode;
		bool installerMenue;
		uint8_t motorPosition;
		int32_t satellitePosition;
		int satfindpid;

		void paint();
		void paintHead();
		void paintMenu(void);
		void paintStatus(void);
		void paintLine(int x, int * y, int width, const char * txt, uint8_t color, uint8_t bgcolor);
		void paintLine(int x, int y, int width, const char * txt, uint8_t color);
		void paintSeparator(int xpos, int * ypos, int width, const char * txt);
		void motorStepEast(void);
		void motorStepWest(void);
		void startSatFind(void);
		void stopSatFind(void);

	public:

		CMotorControl();
		void hide();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif
