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

/*
$Id: streaminfo.cpp,v 1.22 2002/04/10 19:56:19 obi Exp $

Module StreamInfo

History:
 $Log: streaminfo.cpp,v $
 Revision 1.22  2002/04/10 19:56:19  obi
 changed pid defines

 Revision 1.21  2002/03/22 17:34:04  field
 Massive Umstellungen - NVODs/SubChannels=KAPUTT!
 Infoviewer tw. kaputt! NON-STABLE!

 Revision 1.20  2002/03/06 11:18:39  field
 Fixes & Updates

 Revision 1.19  2002/02/27 22:51:13  field
 Tasten kaputt gefixt - sollte wieder gehen :)

 Revision 1.18  2002/02/26 17:24:16  field
 Key-Handling weiter umgestellt EIN/AUS= KAPUTT!

 Revision 1.17  2002/02/25 19:32:26  field
 Events <-> Key-Handling umgestellt! SEHR BETA!

 Revision 1.16  2002/02/25 01:27:33  field
 Key-Handling umgestellt (moeglicherweise beta ;)

 Revision 1.15  2002/01/28 20:50:42  field
 Streaminfo besser

 Revision 1.14  2002/01/28 19:52:32  field
 Streaminfo ausfuehrlicher

 Revision 1.13  2002/01/03 20:03:20  McClean
 cleanup

 Revision 1.12  2001/11/26 02:34:04  McClean
 include (.../../stuff) changed - correct unix-formated files now

 Revision 1.11  2001/11/15 11:42:41  McClean
 gpl-headers added

 Revision 1.10  2001/10/22 15:24:48  McClean
 small designupdate

 Revision 1.9  2001/10/09 21:48:37  McClean
 ucode-check

 Revision 1.8  2001/09/23 21:34:07  rasc
 - LIFObuffer Module, pushbackKey fuer RCInput,
 - In einige Helper und widget-Module eingebracht
   ==> harmonischeres Menuehandling
 - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)


*/


#include "streaminfo.h"
#include "../global.h"

CStreamInfo::CStreamInfo()
{
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
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CStreamInfo::paint()
{
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight+1, width, g_Locale->getText("streaminfo.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

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
