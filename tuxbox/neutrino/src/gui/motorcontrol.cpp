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

CMotorControl::CMotorControl()
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 500;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight + (10*mheight);		//space for infolines
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

int CMotorControl::exec(CMenuTarget* parent, string)
{
	uint msg;
	uint data;
	bool istheend = false;
	
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;

	frameBuffer->loadPal("satellites.pal", 0, 255);
	frameBuffer->loadPicture2FrameBuffer("satellites.raw");

	paint();

	while (!istheend)
	{
		unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd_MS(250);
		msg = CRCInput::RC_nokey;

		while (!(msg == CRCInput::RC_timeout) && (!(msg == CRCInput::RC_home)))
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			switch(msg)
			{

				default:
					printf("[motorcontrol] message received...\n");
					if ((msg >= CRCInput::RC_WithData) && (msg< CRCInput::RC_WithData+ 0x10000000)) 
						delete (unsigned char*) data;
					break;
			}
		}
		if (msg == CRCInput::RC_home)
			istheend = true;
	}
	
	hide();

	return menu_return::RETURN_REPAINT;
}

void CMotorControl::hide()
{
	frameBuffer->loadPal("motorcontrol.pal", 0, 255);
	frameBuffer->paintBackgroundBoxRel(0,0, 720,576);
}


void CMotorControl::paint()
{
	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("motorcontrol.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
}
