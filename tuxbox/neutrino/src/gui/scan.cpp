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


#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>

#include "color.h"
#include "scan.h"

#include "widget/menue.h"
#include "widget/messagebox.h"


CScanTs::CScanTs()
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 500;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+8*mheight;		//space for infolines
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

int CScanTs::exec(CMenuTarget* parent, string)
{
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;

	frameBuffer->loadPal("scan.pal", 37, COL_MAXFREE);
	frameBuffer->loadPicture2FrameBuffer("scan.raw");

	g_Sectionsd->setPauseScanning( true );

	g_Zapit->setDiseqcType( CNeutrinoApp::getInstance()->getScanSettings().diseqcMode);
	g_Zapit->setDiseqcRepeat( CNeutrinoApp::getInstance()->getScanSettings().diseqcRepeat);
	g_Zapit->setScanBouquetMode( CNeutrinoApp::getInstance()->getScanSettings().bouquetMode);

	CZapitClient::ScanSatelliteList satList;
	CNeutrinoApp::getInstance()->getScanSettings().toSatList( satList);
	g_Zapit->setScanSatelliteList( satList);

	bool success = g_Zapit->startScan();

	paint();

	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
	ypos= y+ hheight + (mheight >>1);
	int xpos3 = 0;;
	if (g_info.delivery_system == DVB_S)
	{	//sat only
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.actsatellite").c_str(), COL_MENUCONTENT);
		xpos3 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.actsatellite").c_str());
	}
	if (g_info.delivery_system == DVB_C)		// maybe add DVB_T later:)
	{	//cable
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.actcable").c_str(), COL_MENUCONTENT);
		xpos3 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.actcable").c_str());
	}
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.transponders").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.freqdata").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	ypos+= mheight;	//providername
	ypos+= mheight; // channelname

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.servicenames").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	int xpos1 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.transponders").c_str());
	//int xpos2 = x+20 + g_Fonts->menu->getRenderWidth(g_Locale->getText("scants.services").c_str());


	frameBuffer->loadPal("radar.pal", 17, 37);
	int pos = 0;

	ypos= y+ hheight + (mheight >>1);

	uint msg;
	uint data;
	uint found_transponder = 0;
	bool istheend = !success;

	while (!istheend)
	{
		char filename[30];
		char cb[10];
		char cb1[21];
		sprintf(filename, "radar%d.raw", pos);
		pos = (pos+1)%10;
		frameBuffer->paintIcon8(filename, x+400,ypos+15, 17);

		unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd_MS( 250 );
		msg = CRCInput::RC_nokey;

		while ( ! ( msg == CRCInput::RC_timeout ) )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

			switch (msg)
			{
				case NeutrinoMessages::EVT_SCAN_SATELLITE:
					frameBuffer->paintBoxRel(xpos3, ypos, x+width-400, mheight, COL_MENUCONTENT); //new position set
					g_Fonts->menu->RenderString(xpos3, ypos+ mheight, width, (char*)data, COL_MENUCONTENT);
					delete (unsigned char*) data;
					break;
	//todo: merge the follwing 2 cases:
				case NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS:	//willbe obsolete soon
					sprintf(cb, "%d", data);
					frameBuffer->paintBoxRel(xpos1, ypos+1*mheight, x+width-400, mheight, COL_MENUCONTENT);
					g_Fonts->menu->RenderString(xpos1, ypos+ 2*mheight, width, cb, COL_MENUCONTENT);
					found_transponder = data;
					break;
				case NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
					if (found_transponder == 0) data = 0;
					sprintf(cb1, "%d/%d", data,found_transponder);
					frameBuffer->paintBoxRel(xpos1, ypos+1*mheight, x+width-400, mheight, COL_MENUCONTENT);
					g_Fonts->menu->RenderString(xpos1, ypos+ 2*mheight, width, cb1, COL_MENUCONTENT);
					break;

				case NeutrinoMessages::EVT_SCAN_PROVIDER:
					frameBuffer->paintBoxRel(x+ 10, ypos+ 4* mheight+2, width-20, mheight, COL_MENUCONTENT);
					g_Fonts->menu->RenderString(x+ 10, ypos+ 5* mheight, width-20, (char*)data, COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
					delete (unsigned char*) data;
					break;

				case NeutrinoMessages::EVT_SCAN_NUM_CHANNELS:
					sprintf(cb, "%d", data);
					frameBuffer->paintBoxRel(x+ 10, ypos+6*mheight, width-20, mheight , COL_MENUCONTENT);
					g_Fonts->menu->RenderString(x + 10, ypos+ 7* mheight, width, cb, COL_MENUCONTENT);
					break;

				case NeutrinoMessages::EVT_SCAN_COMPLETE:
				case NeutrinoMessages::EVT_SCAN_FAILED:
    					success  = (msg == NeutrinoMessages::EVT_SCAN_COMPLETE);
					istheend = true;
					msg      = CRCInput::RC_timeout;
					break;
				default:
					if ((msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) ) delete (unsigned char*) data;
					break;
			}
		}
	}

	hide();
	g_Sectionsd->setPauseScanning(false);
	ShowMsg("messagebox.info", success ? g_Locale->getText("scants.finished") : g_Locale->getText("scants.failed"), CMessageBox::mbBack, CMessageBox::mbBack, "info.raw", 450, -1, true); // UTF-8

	return menu_return::RETURN_REPAINT;
}

void CScanTs::hide()
{
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->paintBackgroundBoxRel(0,0, 720,576);
}


void CScanTs::paint()
{
	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("scants.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
}
