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

#include "streaminfo.h"
#include "../global.h"

CStreamInfo::CStreamInfo()
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 350;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+10*mheight+ 10;

    x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}


int CStreamInfo::exec(CMenuTarget* parent, string)
{
	int res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}
	paint();

	bool doLoop = true;

	uint msg; uint data;
	while ( doLoop )
	{
		g_RCInput->getMsg( &msg, &data, 130 );

        if ( msg == CRCInput::RC_timeout )
			doLoop = false;
		else
		{
			int mr = neutrino->handleMsg( msg, data );

			if ( mr & messages_return::cancel_all )
			{
				res = menu_return::RETURN_EXIT_ALL;
				doLoop = false;
			}
			else if ( mr & messages_return::unhandled )
				doLoop = false;
		}

	}

	hide();
	return res;
}

void CStreamInfo::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CStreamInfo::paint()
{
	int ypos=y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight+1, width, g_Locale->getText("streaminfo.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);


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
	sprintf((char*) buf, "%s: %dx%d", g_Locale->getText("streaminfo.resolution").c_str(), bitInfo[0], bitInfo[1] );
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;

	sprintf((char*) buf, "%s: %d bit/sec", g_Locale->getText("streaminfo.bitrate").c_str(), bitInfo[4]*50);
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[2] )
	{
			case 2:
			sprintf((char*) buf, "%s: 4:3", g_Locale->getText("streaminfo.aratio").c_str() );
			break;
			case 3:
			sprintf((char*) buf, "%s: 16:9", g_Locale->getText("streaminfo.aratio").c_str());
			break;
			case 4:
			sprintf((char*) buf, "%s: 2.21:1", g_Locale->getText("streaminfo.aratio").c_str());
			break;
			default:
			sprintf((char*) buf, "%s", g_Locale->getText("streaminfo.aratio_unknown").c_str());
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[3] )
	{
			case 3:
			sprintf((char*) buf, "%s: 25fps", g_Locale->getText("streaminfo.framerate").c_str());
			break;
			case 6:
			sprintf((char*) buf, "%s: 50fps", g_Locale->getText("streaminfo.framerate").c_str());
			break;
			default:
			sprintf((char*) buf, "%s", g_Locale->getText("streaminfo.framerate_unknown").c_str());
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[6] )
	{
			case 1:
			sprintf((char*) buf, "%s: single channel", g_Locale->getText("streaminfo.audiotype").c_str());
			break;
			case 2:
			sprintf((char*) buf, "%s: dual channel", g_Locale->getText("streaminfo.audiotype").c_str());
			break;
			case 3:
			sprintf((char*) buf, "%s: joint stereo", g_Locale->getText("streaminfo.audiotype").c_str());
			break;
			case 4:
			sprintf((char*) buf, "%s: stereo", g_Locale->getText("streaminfo.audiotype").c_str());
			break;
			default:
			sprintf((char*) buf, "%s", g_Locale->getText("streaminfo.audiotype_unknown").c_str());
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight+ 10;

	if ( g_RemoteControl->current_PIDs.PIDs.vpid == 0 )
		sprintf((char*) buf, "%s: %s", "v_pid", g_Locale->getText("streaminfo.not_available").c_str() );
	else
		sprintf((char*) buf, "%s: 0x%x", "v_pid", g_RemoteControl->current_PIDs.PIDs.vpid );
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);
	ypos+= mheight;

	if ( g_RemoteControl->current_PIDs.APIDs.size() == 0 )
		sprintf((char*) buf, "%s: %s", "a_pid(s)", g_Locale->getText("streaminfo.not_available").c_str() );
	else
	{
		sprintf((char*) buf, "%s: ", "a_pid(s)" );
		for (int i= 0; i< g_RemoteControl->current_PIDs.APIDs.size(); i++)
		{
			sprintf((char*) buf2, " 0x%x",  g_RemoteControl->current_PIDs.APIDs[i].pid );

			if (i > 0)
			strcat((char*) buf, ",");

			strcat((char*) buf, buf2);
		}
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);
	ypos+= mheight;

	sprintf((char*) buf, "%s: ", "ecm_pid");
	switch ( g_RemoteControl->current_PIDs.PIDs.ecmpid )
	{
		case NONE :
			strcat((char*) buf, g_Locale->getText("streaminfo.not_crypted").c_str() );
			break;
		case INVALID :
			strcat((char*) buf, g_Locale->getText("streaminfo.ecm_invalid").c_str() );
			break;
		default:
			if ( g_RemoteControl->current_PIDs.PIDs.ecmpid == 0 )
				sprintf((char*) buf, "%s: %s", "ecm_pid", g_Locale->getText("streaminfo.not_available").c_str() );
			else
				sprintf((char*) buf, "%s: 0x%x", "ecm_pid", g_RemoteControl->current_PIDs.PIDs.ecmpid );
	}

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);
	ypos+= mheight;

	if ( g_RemoteControl->current_PIDs.PIDs.vtxtpid == 0 )
        	sprintf((char*) buf, "%s: %s", "vtxt_pid", g_Locale->getText("streaminfo.not_available").c_str() );
	else
        	sprintf((char*) buf, "%s: 0x%x", "vtxt_pid", g_RemoteControl->current_PIDs.PIDs.vtxtpid );
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);
	ypos+= mheight;
}
