/*
	$Id: motorcontrol.cpp,v 1.22 2008/11/16 21:46:40 seife Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/motorcontrol.h>

#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <gui/color.h>

#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>

#include <zapit/client/zapittools.h>

#include <system/settings.h>

#include <global.h>
#include <neutrino.h>

#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>


CMotorControl::CMotorControl()
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	sheight		= g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();

	satfindpid = -1;	

	head_height = hheight + 4;
	status_height =  6*mheight + 4;
	menue_height = 13*sheight;
	
	width = w_max (550, 30);
	height = h_max (head_height + status_height + menue_height, 30);
	
	x = ((720 - width) >> 1);
	y = (576 - height) >> 1;
	
	stepSize = 1; //default: 1 step
	stepMode = STEP_MODE_TIMED;
	installerMenue = false;
	motorPosition = 1;
	satellitePosition = 0;
	stepDelay = 10;
}

int CMotorControl::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	bool istheend = false;
	
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;
	
	if (parent)
		parent->hide();
		
	startSatFind();
		
	paint();
	//paintMenu();
	//paintStatus();

	while (!istheend)
	{

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd_MS(250);
		msg = CRCInput::RC_nokey;

		while (!(msg == CRCInput::RC_timeout) && (!(msg == CRCInput::RC_home)))
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if (installerMenue)
			{
				switch(msg)
				{
					case CRCInput::RC_ok:
					case CRCInput::RC_0:
						printf("[motorcontrol] 0 key received... goto userMenue\n");
						installerMenue = false;
						paintMenu();
						paintStatus();
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_right:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						motorStepWest();
						paintStatus();
						break;
					
					case CRCInput::RC_red:
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_left:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						motorStepEast();
						paintStatus();
						break;
						
					case CRCInput::RC_4:
						printf("[motorcontrol] 4 key received... set west (soft) limit\n");
						g_Zapit->sendMotorCommand(0xE1, 0x31, 0x67, 0, 0, 0);
						break;
						
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... disable (soft) limits\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x63, 0, 0, 0);
						break;
					
					case CRCInput::RC_6:
						printf("[motorcontrol] 6 key received... set east (soft) limit\n");
						g_Zapit->sendMotorCommand(0xE1, 0x31, 0x66, 0, 0, 0);
						break;
					
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto reference position\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6B, 1, 0, 0);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_8:
						printf("[motorcontrol] 8 key received... enable (soft) limits\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6A, 1, 0, 0);
						break;
					
					case CRCInput::RC_9:
						printf("[motorcontrol] 9 key received... (re)-calculate positions\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6F, 1, 0, 0);
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
						if (++stepMode > 2) 
							stepMode = 0;
						if (stepMode == STEP_MODE_OFF)
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
					case CRCInput::RC_ok:
					case CRCInput::RC_0:
						printf("[motorcontrol] 0 key received... goto installerMenue\n");
						installerMenue = true;
						paintMenu();
						paintStatus();
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_right:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						motorStepWest();
						paintStatus();
						break;
					
					case CRCInput::RC_red:
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_left:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						motorStepEast();
						paintStatus();
						break;
					
					case CRCInput::RC_green:
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... store present satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6A, 1, motorPosition, 0);
						break;
					
					case CRCInput::RC_6:
						if (stepSize < 0x7F) stepSize++;
						printf("[motorcontrol] 6 key received... increase Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case CRCInput::RC_yellow:
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6B, 1, motorPosition, 0);
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
						if (++stepMode > 2) 
							stepMode = 0;
						if (stepMode == STEP_MODE_OFF)
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

void CMotorControl::motorStepWest(void)
{
	printf("[motorcontrol] motorStepWest\n");
	switch(stepMode)
	{
		case STEP_MODE_ON:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x69, 1, (-1 * stepSize), 0);
			satellitePosition += stepSize;
			break;
		case STEP_MODE_TIMED:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x69, 1, 40, 0);
			usleep(stepSize * stepDelay * 1000);
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0); //halt motor
			satellitePosition += stepSize;
			break;
		default:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x69, 1, 40, 0);
	}
}	

void CMotorControl::motorStepEast(void)
{
	printf("[motorcontrol] motorStepEast\n");
	switch(stepMode)
	{
		case STEP_MODE_ON:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x68, 1, (-1 * stepSize), 0);
			satellitePosition -= stepSize;
			break;
		case STEP_MODE_TIMED:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x68, 1, 40, 0);
			usleep(stepSize * stepDelay * 1000);
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0); //halt motor
			satellitePosition -= stepSize;
			break;
		default:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x68, 1, 40, 0);
	}
}

void CMotorControl::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height + 20);
	stopSatFind();
}

void CMotorControl::paintLine(int x, int * y, int width, const char * txt, uint8_t color = COL_MENUCONTENT, uint8_t bgcolor = COL_MENUCONTENT_PLUS_0 )
{
	*y += mheight;
	frameBuffer->paintBoxRel(x, *y - mheight, width, mheight, bgcolor);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x, *y, width, txt, color);
}

void CMotorControl::paintLine(int x, int y, int width, const char * txt, uint8_t color = COL_MENUCONTENT)
{
	//frameBuffer->paintBoxRel(x, y - mheight, width, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x, y, width, txt, color);
}

void CMotorControl::paintSeparator(int xpos, int * ypos, int width, const char * txt)
{
	int stringwidth = 0;
	int stringstartposX = 0;
	int offset = 20;
	
	*ypos += mheight;
	frameBuffer->paintHLineRel(xpos, width - offset, *ypos - (mheight >> 1), COL_MENUCONTENT_PLUS_3);
	frameBuffer->paintHLineRel(xpos, width - offset, *ypos - (mheight >> 1) + 1, COL_MENUCONTENT_PLUS_1);
	
	stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(txt);
	stringstartposX = (xpos + (width >> 1)) - (stringwidth >> 1)- (offset >> 1);
	frameBuffer->paintBoxRel(stringstartposX - 5, *ypos - mheight, stringwidth + 10, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, *ypos, stringwidth, txt, COL_MENUCONTENTINACTIVE);
}

void CMotorControl::paint()
{
	paintHead();
	paintStatus();
	paintMenu();
}

void CMotorControl::paintHead()
{
	ypos = y;
	frameBuffer->paintBoxRel(x, ypos, width, head_height, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 10, ypos + head_height-2, width, g_Locale->getText(LOCALE_MOTORCONTROL_HEAD), COL_MENUHEAD, 0, true); // UTF-8
}


void CMotorControl::paintStatus()
{
	char buf[256];
	char buf2[256];
	
	int xpos1 = x + 10;
	int xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("(a) Motor Position:");
	int width2 = width - (xpos2 - xpos1) - 10;
	int width1 = width - 10;
		
	int ypos_status = y + head_height;
	
	// status background
	frameBuffer->paintBoxRel(x, ypos_status , width, status_height, COL_MENUCONTENT_PLUS_0);
	
	// separator
	paintSeparator(xpos1, &ypos_status, width, ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_MOTORCONTROL_SETTINGS)).c_str());
	
	// settings
	ypos_status += mheight+2;
	paintLine(xpos1, ypos_status, width1, "(a) Motor Position:", COL_MENUCONTENTINACTIVE);

	sprintf(buf, "%d", motorPosition);
	paintLine(xpos2, ypos_status, width2 , buf);
	
	paintLine(xpos1, &ypos_status, width1, "(b) Movement:", COL_MENUCONTENTINACTIVE);
	switch(stepMode)
	{
		case STEP_MODE_ON:
			strcpy(buf, "Step Mode");
			break;
		case STEP_MODE_OFF:
			strcpy(buf, "Drive Mode");
			break;
		case STEP_MODE_TIMED:
			strcpy(buf, "Timed Step Mode");
			break;
	}
	paintLine(xpos2, ypos_status, width2, buf);
	
	paintLine(xpos1, &ypos_status, width1, "(c) Step Size:", COL_MENUCONTENTINACTIVE);
	switch(stepMode)
	{
		case STEP_MODE_ON:
			sprintf(buf, "%d", stepSize);
			break;
		case STEP_MODE_OFF:
			strcpy(buf, "don't care");
			break;
		case STEP_MODE_TIMED:
			sprintf(buf, "%d", stepSize * stepDelay);
			strcat(buf, " milliseconds");
			break;
	}
	paintLine(xpos2, ypos_status, width2, buf);
	
	
	// status
	paintSeparator(xpos1, &ypos_status, width, "Status");
	strcpy(buf, "Satellite Position (Step Mode): ");
	sprintf(buf2, "%d", satellitePosition);
	strcat(buf, buf2);
	paintLine(xpos1, &ypos_status, width1, buf, COL_MENUCONTENTINACTIVE);
}

struct button_label CMotorControlMenueButtons1[3] =
{
	{ NEUTRINO_ICON_BUTTON_0			,	 }, // empty caption
	{ NEUTRINO_ICON_BUTTON_OKAY		,  LOCALE_MOTORCONTROL_USER_MENUE },
	{ NEUTRINO_ICON_BUTTON_HOME	,  LOCALE_MOTORCONTROL_EXIT }
};

const struct button_label CMotorControlMenueButtons2[2] =
{
	{ NEUTRINO_ICON_BUTTON_LEFT  ,	LOCALE_MOTORCONTROL_STEP_DRIVE_MOTOR_EAST },
	{ NEUTRINO_ICON_BUTTON_RIGHT, 	LOCALE_MOTORCONTROL_STEP_DRIVE_MOTOR_WEST }
};

const struct button_label CMotorControlMenueButtons3[1] =
{
	{ NEUTRINO_ICON_BUTTON_RED   ,		LOCALE_MOTORCONTROL_STEP_DRIVE_MOTOR_HALT }
};

struct button_label CMotorControlMenueButtons4[9] =
{
	{ NEUTRINO_ICON_BUTTON_4       ,		LOCALE_MOTORCONTROL_SET_WEST_SOFT_LIMIT },
	{ NEUTRINO_ICON_BUTTON_5       ,		LOCALE_MOTORCONTROL_DISABLE_SOFT_LIMITS },
	{ NEUTRINO_ICON_BUTTON_6       ,		LOCALE_MOTORCONTROL_SET_EAST_SOFT_LIMIT },
	{ NEUTRINO_ICON_BUTTON_7       ,		LOCALE_MOTORCONTROL_GOTO_REFERENCE_POSITION },
	{ NEUTRINO_ICON_BUTTON_8       ,		LOCALE_MOTORCONTROL_ENABLE_SOFT_LIMITS },
	{ NEUTRINO_ICON_BUTTON_9       ,		LOCALE_MOTORCONTROL_RE_CALCULATE_POSITIONS }
};

const struct button_label CMotorControlMenueButtons5[3] =
{
	{ NEUTRINO_ICON_BUTTON_TOP	  ,		LOCALE_MOTORCONTROL_INCREASE_MOTOR_POSITION},
	{ NEUTRINO_ICON_BUTTON_DOWN ,	LOCALE_MOTORCONTROL_DECREASE_MOTOR_POSITION},
	{ NEUTRINO_ICON_BUTTON_BLUE  ,		LOCALE_MOTORCONTROL_SWITCH_STEP_DRIVE_MODE}
};

void CMotorControl::paintMenu()
{
	ypos_menue = y + head_height + status_height;
	int buttonwidth = width/2;
	int xposButton = x + 10;
	int font = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;
	
	// rc menue head 
	frameBuffer->paintBoxRel(x, ypos_menue, width, mheight, COL_MENUHEAD_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + 10, ypos_menue+mheight, width, g_Locale->getText(LOCALE_MOTORCONTROL_RC_MENUEHEAD), COL_MENUHEAD, 0, true); // UTF-8

	// rc menue background
	ypos_menue += mheight;
	frameBuffer->paintBoxRel(x, ypos_menue , width, menue_height-mheight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	
	// paint 0/OK and menue caption user menue usr installer menue
	CMotorControlMenueButtons1[1].locale = (installerMenue ? LOCALE_MOTORCONTROL_USER_MENUE : LOCALE_MOTORCONTROL_INSTALLER_MENUE);
	ypos_menue += 10;
	
	// buttons 0 / Ok / home
	::paintButtons(frameBuffer, g_Font[font], g_Locale, xposButton, ypos_menue, buttonwidth-28, 3, CMotorControlMenueButtons1);
	
	// buttons left / right
	ypos_menue += mheight;
	::paintButtons(frameBuffer, g_Font[font], g_Locale, xposButton, ypos_menue, buttonwidth,2, CMotorControlMenueButtons2);
	
	// button red
	ypos_menue += mheight;
	::paintButtons(frameBuffer, g_Font[font], g_Locale, xposButton, ypos_menue, buttonwidth,1, CMotorControlMenueButtons3);
	
	// variant buttons	
	if (installerMenue)
	 {
		 CMotorControlMenueButtons4[0].locale = LOCALE_MOTORCONTROL_SET_WEST_SOFT_LIMIT;
		 CMotorControlMenueButtons4[1].locale = LOCALE_MOTORCONTROL_DISABLE_SOFT_LIMITS;
		 CMotorControlMenueButtons4[2].locale = LOCALE_MOTORCONTROL_ENABLE_SOFT_LIMITS;
		 CMotorControlMenueButtons4[3].locale = LOCALE_MOTORCONTROL_GOTO_REFERENCE_POSITION;
		 CMotorControlMenueButtons4[4].locale = LOCALE_MOTORCONTROL_ENABLE_SOFT_LIMITS;		 
		 CMotorControlMenueButtons4[5].locale = LOCALE_MOTORCONTROL_RE_CALCULATE_POSITIONS;		 
	 }
	else
	{
		CMotorControlMenueButtons4[0].locale = LOCALE_MOTORCONTROL_NOT_DEFINED;
		CMotorControlMenueButtons4[1].locale = LOCALE_MOTORCONTROL_STORE_MOTOR_POSITION;
		CMotorControlMenueButtons4[2].locale = LOCALE_MOTORCONTROL_INCREASE_STEP_SIZE;
		CMotorControlMenueButtons4[3].locale = LOCALE_MOTORCONTROL_GOTO_MOTOR_POSITION;
		CMotorControlMenueButtons4[4].locale = LOCALE_MOTORCONTROL_NOT_DEFINED;		
		CMotorControlMenueButtons4[5].locale = LOCALE_MOTORCONTROL_DECREASE_STEP_SIZE;
	 }
	ypos_menue += mheight;
 	::paintButtons(frameBuffer, g_Font[font], g_Locale, xposButton, ypos_menue, buttonwidth, 6, CMotorControlMenueButtons4, width, true); //vertical	 
	 
	//ypos_menue += mheight;
	::paintButtons(frameBuffer, g_Font[font], g_Locale, xposButton+buttonwidth, ypos_menue, buttonwidth, 3, CMotorControlMenueButtons5, width, true); //vertical
}

void CMotorControl::startSatFind(void)
{
	
		if (satfindpid != -1)
		{
			kill(satfindpid, SIGKILL);
			waitpid(satfindpid, 0, 0);
			satfindpid = -1;
		}
		
		switch ((satfindpid = fork()))
		{
		case -1:
			printf("[motorcontrol] fork");
			break;
		case 0:
			printf("[motorcontrol] starting satfind...\n");
			if (execlp("/bin/satfind", "satfind", NULL) < 0)
				printf("[motorcontrol] execlp satfind failed.\n");		
			break;
		} /* switch */
}

void CMotorControl::stopSatFind(void)
{
	
	if (satfindpid != -1)
	{
		printf("[motorcontrol] killing satfind...\n");
		kill(satfindpid, SIGKILL);
		waitpid(satfindpid, 0, 0);
		satfindpid = -1;
	}
}
