/*
	Neutrino-GUI  -   DBoxII-Project

	Homepage: http://dbox.cyberphoria.org/

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


#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>

#include "color.h"
#include "motorcontrol.h"

#include "widget/menue.h"
#include "widget/messagebox.h"

#include "system/settings.h"

int8_t stepSize = -1; //default: 1 step
bool stepMode = true;
bool installerMenue = false;
uint8_t motorPosition = 0;

CMotorControl::CMotorControl()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	frameBuffer->loadPal("satellites.pal", 0, 255);
	frameBuffer->loadPicture2FrameBuffer("satellites.raw");
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	
	width = 500;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight + (10 * mheight);
	x = ((720 - width) >> 1) - 20;
	y = (576 - height) >> 1;
}

int CMotorControl::exec(CMenuTarget* parent, string)
{
	uint msg;
	uint data;
	bool istheend = false;
	
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;

	while (!istheend)
	{
		paint();
		
		unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd_MS(500);
		msg = CRCInput::RC_nokey;

		while (!(msg == CRCInput::RC_timeout) && (!(msg == CRCInput::RC_home)))
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if (installerMenue)
			{
				switch(msg)
				{
					case CRCInput::RC_0:
						printf("[motorcontrol] 0 key received... goto userMenue\n");
						installerMenue = false;
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_left:
						printf("[motorcontrol] left/1 key received... drive/nudge motor west, stepMode: %d\n", stepMode);
						if (stepMode)
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, stepSize, 0);
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, 40, 0);
						break;
					
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_right:
						printf("[motorcontrol] right/3 key received... drive/nudge motor east, stepMode: %d\n", stepMode);
						if (stepMode)
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, stepSize, 0);
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, 40, 0);
						break;
						
					case CRCInput::RC_4:
						printf("[motorcontrol] 4 key received... set west (soft) limit\n");
						g_Zapit->sendMotorCommand(0xE1, 0x30, 0x67, 0, 0, 0);
						break;
						
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... disable (soft) limits\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x63, 0, 0, 0);
						break;
					
					case CRCInput::RC_6:
						printf("[motorcontrol] 6 key received... set east (soft) limit\n");
						g_Zapit->sendMotorCommand(0xE1, 0x30, 0x66, 0, 0, 0);
						break;
					
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto reference position\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6B, 1, 0, 0);
						break;
					
					case CRCInput::RC_8:
						printf("[motorcontrol] 8 key received... enable (soft) limits\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6A, 1, 0, 0);
						break;
					
					case CRCInput::RC_9:
						printf("[motorcontrol] 9 key received... (re)-calculate positions\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6F, 1, 0, 0);
						break;
					
					case CRCInput::RC_plus:
					case CRCInput::RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						break;
					
					case CRCInput::RC_minus:
					case CRCInput::RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						break;
					
					case CRCInput::RC_red:
						stepMode = !stepMode;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						break;
					
					default:
						printf("[motorcontrol] message received...\n");
						if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000)) 
							delete (unsigned char*) data;
						break;
				}
			}
			else
			{
				switch(msg)
				{
					case CRCInput::RC_0:
						printf("[motorcontrol] 0 key received... goto installerMenue\n");
						installerMenue = true;
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_left:
						printf("[motorcontrol] left/1 key received... drive/nudge motor west, stepMode: %d\n", stepMode);
						if (stepMode)
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, stepSize, 0);
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, 40, 0);
						break;
					
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_right:
						printf("[motorcontrol] right/3 key received... drive/nudge motor east, stepMode: %d\n", stepMode);
						if (stepMode)
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, stepSize, 0);
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, 40, 0);
						break;
					
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... store present satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6A, 1, motorPosition, 0);
						break;
					
					case CRCInput::RC_6:
						if (stepSize < 0x7F) stepSize--;
						printf("[motorcontrol] 6 key received... increase nudge size: %d\n", stepSize);
						break;
					
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6B, 1, motorPosition, 0);
						break;
					
					case CRCInput::RC_9:
						if (stepSize < -1) stepSize++;
						printf("[motorcontrol] 9 key received... decrease nudge size: %d\n", stepSize);
						break;
					
					case CRCInput::RC_plus:
					case CRCInput::RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						break;
					
					case CRCInput::RC_minus:
					case CRCInput::RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						break;
					
					case CRCInput::RC_red:
						stepMode = !stepMode;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						break;
					
					default:
						printf("[motorcontrol] message received...\n");
						if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000)) 
							delete (unsigned char*) data;
						break;
				}
			}
		}
		
		istheend = (msg == CRCInput::RC_home);
	}
	
	hide();

	return menu_return::RETURN_REPAINT;
}

void CMotorControl::hide()
{
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->paintBackgroundBoxRel(0,0, 720,576);
}


void CMotorControl::paint()
{
	int ypos = y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x + 10, ypos + hheight, width, g_Locale->getText("motorcontrol.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos + hheight, width, height - hheight, COL_MENUCONTENT);
	
	//frameBuffer->paintBoxRel(x+ 8, ypos+ 5* mheight+2, width-x-10, mheight, COL_MENUCONTENT);
	//g_Fonts->menu->RenderString(x+ 10, ypos+ 6* mheight, width-x-10, ("Einstellungen", COL_MENUCONTENT, 0, true);
}
