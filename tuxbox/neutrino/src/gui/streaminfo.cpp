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

#include <gui/streaminfo.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CStreamInfo::CStreamInfo()
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	width       = 400;
	height      = hheight+14*mheight+ 10;

    	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}


int CStreamInfo::exec(CMenuTarget* parent, const std::string &)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int res = g_RCInput->messageLoop();

	hide();
	return res;
}

void CStreamInfo::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CStreamInfo::paint()
{
	const char * head_string;
	int ypos;

	head_string = g_Locale->getText("streaminfo.head");

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, head_string);
	
	ypos = y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10, ypos+ hheight+1, width, head_string, COL_MENUHEAD, 0, true); // UTF-8

	ypos+= hheight;
	frameBuffer->paintBoxRel(x, ypos, width, height - hheight, COL_MENUCONTENT);

	ypos+= (mheight >>1);


	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return;
	}

	int bitInfo[10];

	char *key,*tmpptr,buf[100], buf2[100];
	int value, pos=0;
	fgets(buf,29,fd);//dummy
	while(!feof(fd))
	{
		if(fgets(buf,29,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			key=strsep(&tmpptr,":");
			for(;tmpptr[0]==' ';tmpptr++)
				;
			value=atoi(tmpptr);
			//printf("%s: %d\n",key,value);
			bitInfo[pos]= value;
			pos++;
		}
	}
	fclose(fd);


	//paint msg...
	ypos+= mheight;
	sprintf((char*) buf, "%s: %dx%d", g_Locale->getText("streaminfo.resolution"), bitInfo[0], bitInfo[1] );
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8


	ypos += mheight;
	sprintf((char*) buf, "%s: %d bits/sec", g_Locale->getText("streaminfo.bitrate"), bitInfo[4]*50);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8


	ypos += mheight;
	switch (bitInfo[2])
	{
	case 2:
		sprintf((char*) buf, "%s: 4:3", g_Locale->getText("streaminfo.aratio"));
		break;
	case 3:
		sprintf((char*) buf, "%s: 16:9", g_Locale->getText("streaminfo.aratio"));
		break;
	case 4:
		sprintf((char*) buf, "%s: 2.21:1", g_Locale->getText("streaminfo.aratio"));
		break;
	default:
		strncpy(buf, g_Locale->getText("streaminfo.aratio_unknown"), sizeof(buf));
	}
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8



	ypos+= mheight;
	switch ( bitInfo[3] )
	{
			case 3:
			sprintf((char*) buf, "%s: 25fps", g_Locale->getText("streaminfo.framerate"));
			break;
			case 6:
			sprintf((char*) buf, "%s: 50fps", g_Locale->getText("streaminfo.framerate"));
			break;
			default:
			strncpy(buf, g_Locale->getText("streaminfo.framerate_unknown"), sizeof(buf));
	}
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8



	switch ( bitInfo[6] )
	{
			case 1:
			sprintf((char*) buf, "%s: single channel", g_Locale->getText("streaminfo.audiotype"));
			break;
			case 2:
			sprintf((char*) buf, "%s: dual channel", g_Locale->getText("streaminfo.audiotype"));
			break;
			case 3:
			sprintf((char*) buf, "%s: joint stereo", g_Locale->getText("streaminfo.audiotype"));
			break;
			case 4:
			sprintf((char*) buf, "%s: stereo", g_Locale->getText("streaminfo.audiotype"));
			break;
			default:
			strncpy(buf, g_Locale->getText("streaminfo.audiotype_unknown"), sizeof(buf));
	}
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos+ mheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= mheight+ 10;

	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();

	//onid
	ypos+= mheight;
	sprintf((char*) buf, "%s: 0x%04x", "onid", si.onid);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//sid
	ypos+= mheight;
	sprintf((char*) buf, "%s: 0x%04x", "sid", si.sid);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//tsid
	ypos+= mheight;
	sprintf((char*) buf, "%s: 0x%04x", "tsid", si.tsid);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//tsfrequenz
	ypos+= mheight;
	sprintf((char*) buf, "%s: %dkhz (%c)", "tsf", si.tsfrequency, (si.polarisation == HORIZONTAL) ? 'h' : 'v');
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//vpid
	ypos+= mheight;
	if ( g_RemoteControl->current_PIDs.PIDs.vpid == 0 )
		sprintf((char*) buf, "%s: %s", "vpid", g_Locale->getText("streaminfo.not_available"));
	else
		sprintf((char*) buf, "%s: 0x%04x", "vpid", g_RemoteControl->current_PIDs.PIDs.vpid );
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//apid	
	ypos+= mheight;
	if (g_RemoteControl->current_PIDs.APIDs.empty())
		sprintf((char*) buf, "%s: %s", "apid(s)", g_Locale->getText("streaminfo.not_available"));
	else
	{
		sprintf((char*) buf, "%s: ", "apid(s)" );
		for (unsigned int i= 0; i< g_RemoteControl->current_PIDs.APIDs.size(); i++)
		{
			sprintf((char*) buf2, " 0x%04x",  g_RemoteControl->current_PIDs.APIDs[i].pid );

			if (i > 0)
			{
				strcat((char*) buf, ",");
				strcat((char*) buf, buf2+4);
			}
			else
				strcat((char*) buf, buf2);
		}
	}
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//vtxtpid
	if ( g_RemoteControl->current_PIDs.PIDs.vtxtpid == 0 )
        	sprintf((char*) buf, "%s: %s", "vtxtpid", g_Locale->getText("streaminfo.not_available"));
	else
        	sprintf((char*) buf, "%s: 0x%04x", "vtxtpid", g_RemoteControl->current_PIDs.PIDs.vtxtpid );
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos+ mheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= mheight+ 10;
	
	//satellite
	sprintf((char*) buf, "Provider / Sat: %s",CNeutrinoApp::getInstance()->getScanSettings().satOfDiseqc(si.diseqc));
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, ypos+ mheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
}
