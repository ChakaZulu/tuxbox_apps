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

#include "scan.h"
#include "../global.h"


CScanTs::CScanTs()
{
	width = 400;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

int CScanTs::exec(CMenuTarget* parent, string)
{
	g_FrameBuffer->loadPal("scan.pal", 37, COL_MAXFREE);
	g_FrameBuffer->loadPicture2Mem("scan.raw", g_FrameBuffer->lfb);

	g_Sectionsd->setPauseScanning( true );
	g_Zapit->startScan( g_settings.scan_astra | g_settings.scan_eutel | g_settings.scan_kopernikus | g_settings.scan_digituerk | g_settings.scan_bouquet );

	paint();

	char strServices[100] = "";
	char strTransponders[100] = "";
	char strSatellite[100] = "";
	char strLastServices[100] = "";
	char strLastTransponders[100] = "";
	char strLastSatellite[100] = "";

	unsigned int ts = 0;
	unsigned int services = 0;
	unsigned int sat = 0;
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
	ypos= y+ hheight + (mheight >>1);
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.transponders").c_str(), COL_MENUCONTENT);
	ypos+= mheight;
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.services").c_str(), COL_MENUCONTENT);
	ypos+= mheight;
	if (atoi(getenv("fe"))==1)
	{	//sat only
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.actsatellite").c_str(), COL_MENUCONTENT);
	}

	int xpos1 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.transponders").c_str());
	int xpos2 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.services").c_str());
	int xpos3 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.actsatellite").c_str());

	g_FrameBuffer->loadPal("radar.pal", 17, 37);
	int pos = 0;
	bool finish = false;
	while (!finish)
	{
		if(pos==0)
		{	//query zapit every xth loop
			finish = g_Zapit->isScanReady( sat, ts, services);
		}

		ypos= y+ hheight + (mheight >>1);

		char filename[30];
		sprintf(filename, "radar%d.raw", pos);
		pos = (pos+1)%10;
		g_FrameBuffer->paintIcon8(filename, x+300,ypos+15, 17);

		sprintf(strTransponders, "%d", ts);
		if(strcmp(strLastTransponders, strTransponders)!=0)
		{
			g_FrameBuffer->paintBoxRel(xpos1, ypos, 80, mheight, COL_MENUCONTENT);
			g_Fonts->menu->RenderString(xpos1, ypos+ mheight, width, strTransponders, COL_MENUCONTENT);
			strcpy(strLastTransponders, strTransponders);
		}
		ypos+= mheight;

		sprintf(strServices, "%d", services);
		if(strcmp(strLastServices,strServices)!=0)
		{
			g_FrameBuffer->paintBoxRel(xpos2, ypos, 80, mheight, COL_MENUCONTENT);
			g_Fonts->menu->RenderString(xpos2, ypos+ mheight, width, strServices, COL_MENUCONTENT);
			strcpy(strLastServices,strServices);
		}
		ypos+= mheight;

		if (atoi(getenv("fe"))==1)
		{	//sat only
			switch (sat)
			{
					case 1:
					strcpy(strSatellite, g_Locale->getText("scants.astra").c_str() );
					break;
					case 2:
					strcpy(strSatellite, g_Locale->getText("scants.hotbird").c_str() );
					break;
					case 4:
					strcpy(strSatellite, g_Locale->getText("scants.kopernikus").c_str() );
					break;
					case 8:
					strcpy(strSatellite, g_Locale->getText("scants.digituerk").c_str() );
					break;
					default:
					strcpy(strSatellite,"unknown");
			}
			if(strcmp(strLastSatellite,strSatellite)!=0)
			{
				g_FrameBuffer->paintBoxRel(xpos3, ypos, 80, mheight, COL_MENUCONTENT);
				g_Fonts->menu->RenderString(xpos3, ypos+ mheight, width, strSatellite, COL_MENUCONTENT);
				strcpy(strLastSatellite,strSatellite);
			}
		}

		//g_RCInput->getKey(190);
		usleep(100000);
	}


	hide();
	g_RCInput->postMsg( messages::EVT_SERVICESCHANGED, 0 );
	g_Sectionsd->setPauseScanning( false );
	return menu_return::RETURN_REPAINT;
}

void CScanTs::hide()
{
	g_FrameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	g_FrameBuffer->paintBackgroundBoxRel(0,0, 720,576);
}


void CScanTs::paint()
{
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("scants.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
}
