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


#include <stdlib.h>
#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>

#include "color.h"
#include "motorcontrol.h"

#include "widget/menue.h"
#include "widget/messagebox.h"

#include "system/settings.h"

CMotorControl::CMotorControl()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	width = 420;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight + (18 * mheight);
	x = ((720 - width) >> 1);
	y = (576 - height) >> 1;
	
	stepSize = 1; //default: 1 step
	stepMode = true;
	installerMenue = false;
	motorPosition = 1;
	satellitePosition = 0;
}

int CMotorControl::exec(CMenuTarget* parent, string)
{
	uint msg;
	uint data;
	bool istheend = false;
	
	
	//frameBuffer->loadPal("satellites.pal", 0, 255);
	//frameBuffer->loadPicture2FrameBuffer("satellites.raw");
	//frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	
	
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;
		
	paint();
	paintMenu();
	paintStatus();

	while (!istheend)
	{

		unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd_MS(250);
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
						paintMenu();
						paintStatus();
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_left:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						if (stepMode)
						{
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, (-1 * stepSize), 0);
							satellitePosition -= stepSize;
						}
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, 40, 0);
						paintStatus();
						break;
					
					case CRCInput::RC_red:
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_right:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						if (stepMode)
						{
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, (-1 * stepSize), 0);
							satellitePosition += stepSize;
						}
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, 40, 0);
						paintStatus();
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
						satellitePosition = 0;
						paintStatus();
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
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_minus:
					case CRCInput::RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_blue:
						stepMode = !stepMode;
						if (!stepMode)
							satellitePosition = 0;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						paintStatus();
						break;
					
					default:
						//printf("[motorcontrol] message received...\n");
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
						paintMenu();
						paintStatus();
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_left:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						if (stepMode)
						{
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, (-1 * stepSize), 0);
							satellitePosition -= stepSize;
						}
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x69, 1, 40, 0);
						paintStatus();
						break;
					
					case CRCInput::RC_red:
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_right:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						if (stepMode)
						{
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, (-1 * stepSize), 0);
							satellitePosition += stepSize;
						}
						else
							g_Zapit->sendMotorCommand(0xE1, 0x31, 0x68, 1, 40, 0);
						paintStatus();
						break;
					
					case CRCInput::RC_green:
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... store present satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6A, 1, motorPosition, 0);
						break;
					
					case CRCInput::RC_6:
						if (stepSize < 0x7F) stepSize++;
						printf("[motorcontrol] 6 key received... increase Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case CRCInput::RC_yellow:
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x30, 0x6B, 1, motorPosition, 0);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_9:
						if (stepSize > 1) stepSize--;
						printf("[motorcontrol] 9 key received... decrease Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case CRCInput::RC_plus:
					case CRCInput::RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_minus:
					case CRCInput::RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_blue:
						stepMode = !stepMode;
						if (!stepMode)
							satellitePosition = 0;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						paintStatus();
						break;
					
					default:
						//printf("[motorcontrol] message received...\n");
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
	frameBuffer->paintBackgroundBoxRel(x, y, width, height + 20);
}

void CMotorControl::paintLine(char * txt, char * icon)
{
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUCONTENT);
	g_Fonts->menu->RenderString(x + 10, ypos + mheight, width, txt, COL_MENUCONTENT);
	ypos += mheight;
}

void CMotorControl::paintStatus()
{
	char buf[256];
	char buf2[256];
	
	ypos = ypos_status;
	paintLine("------ Motor Control Settings ------", NULL);
	
	buf[0] = '\0';
	strcat(buf, "Motor Position: ");
	sprintf(buf2, "%d", motorPosition);
	strcat(buf, buf2);
	paintLine(buf, NULL);
	
	buf[0] = '\0';
	strcat(buf, "Satellite Position: ");
	sprintf(buf2, "%d", satellitePosition);
	strcat(buf, buf2);
	paintLine(buf, NULL);
	
	buf[0] = '\0';
	strcat(buf, "Step Size: ");
	sprintf(buf2, "%d", stepSize);
	strcat(buf, buf2);
	paintLine(buf, NULL);
	
	buf[0] = '\0';
	strcat(buf, "Movement: ");
	if (stepMode)
		strcat(buf, "Step Mode");
	else
		strcat(buf, "Drive Mode");
	paintLine(buf, NULL);
}

void CMotorControl::paint()
{
	ypos = y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x + 10, ypos + hheight + 1, width, g_Locale->getText("motorcontrol.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos + hheight, width, height - hheight, COL_MENUCONTENT);

	ypos += hheight + (mheight >> 1);
	ypos_menue = ypos;
}

void CMotorControl::paintMenu()
{
	ypos = ypos_menue;
	
	if (installerMenue)
	{
		paintLine("(0) User Menue", NULL);
		paintLine("(1/left)) Step/Drive Motor West", NULL);
		paintLine("(2/red) Halt Motor", NULL);
		paintLine("(3/right) Step/Drive Motor East", NULL);
		paintLine("(4) Set West (soft) Limit", NULL);
		paintLine("(5) Disable (soft) Limits", NULL);
		paintLine("(6) Set East (soft) Limit", NULL);
		paintLine("(7) Goto Reference Position", NULL);
		paintLine("(8) Enable (soft) Limits", NULL);
		paintLine("(9) (Re)-Calculate Positions", NULL);
		paintLine("(+/up) Increase Motor Position", NULL);
		paintLine("(-/down) Decrease Motor Position", NULL);
		paintLine("(blue) Toggle Step/Drive Mode", NULL);
	}
	else
	{
		paintLine("(0) Installer Menue", NULL);
		paintLine("(1/left)) Step/Drive Motor West", NULL);
		paintLine("(2/red) Halt Motor", NULL);
		paintLine("(3/right) Step/Drive Motor East", NULL);
		paintLine("(4) not defined", NULL);
		paintLine("(5/green) Store Motor Position", NULL);
		paintLine("(6) Increase Step Size", NULL);
		paintLine("(7/yellow) Goto Motor Position", NULL);
		paintLine("(8) not defined", NULL);
		paintLine("(9) Decrease Step Size", NULL);
		paintLine("(+/up) Increase Motor Position", NULL);
		paintLine("(-/down) Decrease Motor Position", NULL);
		paintLine("(blue) Toggle Step/Drive Mode", NULL);	
	}
	
	ypos_status = ypos;
}




