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

//
// $Id: eventlist.cpp,v 1.44 2002/04/18 10:42:55 field Exp $
//
//  -- EPG Event List // Vorschau
//
//
//
// $Log: eventlist.cpp,v $
// Revision 1.44  2002/04/18 10:42:55  field
// Updates, sectionsd clientlib
//
// Revision 1.43  2002/03/22 17:34:04  field
// Massive Umstellungen - NVODs/SubChannels=KAPUTT!
// Infoviewer tw. kaputt! NON-STABLE!
//
// Revision 1.42  2002/03/13 21:28:19  McClean
// implement phong shading
//
// Revision 1.41  2002/03/06 11:18:39  field
// Fixes & Updates
//
// Revision 1.40  2002/02/28 15:03:55  field
// Weiter Updates :)
//
// Revision 1.39  2002/02/27 22:51:13  field
// Tasten kaputt gefixt - sollte wieder gehen :)
//
// Revision 1.38  2002/02/26 17:24:16  field
// Key-Handling weiter umgestellt EIN/AUS= KAPUTT!
//
// Revision 1.37  2002/02/25 19:32:26  field
// Events <-> Key-Handling umgestellt! SEHR BETA!
//
// Revision 1.36  2002/02/25 01:27:33  field
// Key-Handling umgestellt (moeglicherweise beta ;)
//
// Revision 1.35  2002/02/04 06:15:30  field
// sectionsd interface verbessert (bug beseitigt)
//
// Revision 1.34  2002/01/31 12:41:02  field
// dd-availibility, eventlist-beginnt in...
//
// Revision 1.31  2002/01/16 00:28:30  McClean
// cleanup
//
// Revision 1.30  2002/01/15 22:08:13  McClean
// cleanups
//
// Revision 1.29  2002/01/03 20:03:20  McClean
// cleanup
//
// Revision 1.28  2001/12/12 19:11:32  McClean
// prepare timing setup...
//
// Revision 1.27  2001/12/12 01:47:17  McClean
// cleanup
//
// Revision 1.26  2001/11/26 02:34:04  McClean
// include (.../../stuff) changed - correct unix-formated files now
//
// Revision 1.25  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.24  2001/11/05 17:13:26  field
// wiederholungen...?
//
// Revision 1.23  2001/11/03 15:43:17  field
// Perspektiven
//
// Revision 1.22  2001/11/03 03:13:10  field
// EPG Anzeige verbessert
//
// Revision 1.21  2001/10/31 12:51:34  field
// bugfix, wenn kein aktuelles event
//
// Revision 1.20  2001/10/31 12:35:39  field
// sectionsd stoppen waehrend scan
//
// Revision 1.19  2001/10/29 16:49:00  field
// Kleinere Bug-Fixes (key-input usw.)
//
// Revision 1.18  2001/10/22 15:24:48  McClean
// small designupdate
//
// Revision 1.17  2001/10/18 22:01:31  field
// kleiner Bugfix
//
// Revision 1.16  2001/10/18 14:31:23  field
// Scrollleisten :)
//
// Revision 1.15  2001/10/14 14:30:47  rasc
// -- EventList Darstellung ueberarbeitet
// -- kleiner Aenderungen und kleinere Bugfixes
// -- locales erweitert..
//
// Revision 1.14  2001/10/11 21:04:58  rasc
// - EPG:
//   Event: 2 -zeilig: das passt aber noch nicht  ganz (read comments!).
//   Key-handling etwas harmonischer gemacht  (Left/Right/Exit)
// - Code etwas restrukturiert und eine Fettnaepfe meinerseits beseitigt
//   (\r\n wg. falscher CSV Einstellung...)
//
// Revision 1.13  2001/10/04 19:28:44  fnbrd
// Eventlist benutzt ID bei zapit und laesst sich per rot wieder schliessen.
//
// Revision 1.12  2001/09/26 09:57:03  field
// Tontraeger-Auswahl ok (bei allen Chans. auf denen EPG geht)
//
// Revision 1.11  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.10  2001/09/20 17:02:16  field
// event-liste zeigt jetzt auch epgs an...
//
// Revision 1.9  2001/09/20 11:55:58  fnbrd
// removed warning.
//
// Revision 1.8  2001/09/20 00:36:32  field
// epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
// Revision 1.7  2001/09/19 00:11:57  McClean
// dont add events with "" Text
//
// Revision 1.6  2001/09/18 20:20:26  field
// Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
// vorbereitet
//
// Revision 1.5  2001/09/18 15:26:09  field
// Handelt nun auch "kein epg"
//
// Revision 1.4  2001/09/18 14:58:20  field
// Eventlist verbessert
//
// Revision 1.3  2001/09/18 11:34:42  fnbrd
// Some changes.
//
// Revision 1.2  2001/09/18 11:05:58  field
// Ausgabe quick'n'dirty gefixt
//
// Revision 1.1  2001/09/18 10:50:30  fnbrd
// Eventlist, quick'n dirty
//
//

#include "eventlist.hpp"
#include "../global.h"

void EventList::readEvents(unsigned onidSid, const std::string& channelname)
{
	current_event = (unsigned int)-1;
	evtlist = g_Sectionsd->getEventsServiceKey( onidSid );
    time_t azeit=time(NULL);

	for ( CChannelEventList::iterator e= evtlist.begin(); e != evtlist.end(); ++e )
	{
    	if ( e->startTime > azeit )
    		break;
    	current_event++;
	}

	if ( evtlist.size() == 0 )
	{
		CChannelEvent evt;

		evt.description= g_Locale->getText("epglist.noevents") ;
		evt.eventID = 0;
		evtlist.insert(evtlist.end(), evt);

	}
	if (current_event == (unsigned int)-1)
		current_event = 0;
	selected= current_event;

	return;
}

EventList::EventList()
{
	selected = 0;
	current_event = 0;

	width = 580;
	//	height = 440;
	height = 480;
	theight= g_Fonts->eventlist_title->getHeight();
	fheight1= g_Fonts->eventlist_itemLarge->getHeight();
	{
		int h1,h2;
		h1 = g_Fonts->eventlist_itemSmall->getHeight();
		h2 = g_Fonts->eventlist_datetime->getHeight();
		fheight2 = (h1 > h2) ? h1 : h2;
	}
	fheight = fheight1 + fheight2 + 2;
	fwidth1 = g_Fonts->eventlist_datetime->getRenderWidth("DDD, 00:00,  ");
	fwidth2 = g_Fonts->eventlist_itemSmall->getRenderWidth("[999 min] ");


	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
}


EventList::~EventList()
{
}

int EventList::exec(unsigned onidSid, const std::string& channelname)
{
	int res = menu_return::RETURN_REPAINT;

	name = channelname;
	paintHead();
	readEvents(onidSid, channelname);
	paint();

	int oldselected = selected;
	bool loop=true;

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "epg-Eventlist: %08x \"%s\"", onidSid, channelname.c_str() );
		g_ActionLog->println(buf);
	#endif


	while (loop)
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, g_settings.timing_chanlist );

		if ( msg == g_settings.key_channelList_pageup )
		{
			selected+=listmaxshow;
			if (selected>evtlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == g_settings.key_channelList_pagedown )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=evtlist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_up )
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = evtlist.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ( msg == CRCInput::RC_down )
		{
			int prevselected=selected;
			selected = (selected+1)%evtlist.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ( ( msg == CRCInput::RC_timeout ) ||
			 	  ( msg == g_settings.key_channelList_cancel ) )
		{
			selected = oldselected;
			loop=false;
		}

		else if ( ( msg == CRCInput::RC_ok ) ||
				  ( msg == CRCInput::RC_left ) ||
				  ( msg == CRCInput::RC_red ) )
		{
			loop= false;
		}
		else if (msg==CRCInput::RC_help || msg==CRCInput::RC_right)
		{
			if ( evtlist[selected].eventID != 0 )
			{
				hide();

				res = g_EpgData->show(channelname, onidSid, evtlist[selected].eventID, &evtlist[selected].startTime);
                if ( res == menu_return::RETURN_EXIT_ALL )
                {
                	loop = false;
                }
                else
                {
                	g_RCInput->getMsg( &msg, &data, 0 );

					if ( ( msg != CRCInput::RC_red ) &&
				         ( msg != CRCInput::RC_timeout ) )
					{
						// RC_red schlucken
						g_RCInput->postMsg( msg, data );
					}

					paintHead();
					paint();
				}
			}
		}
		else
		{
			if ( neutrino->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = menu_return::RETURN_EXIT_ALL;
			}
		}
	}

	hide();

	#ifdef USEACTIONLOG
		g_ActionLog->println("epg-Eventlist: closed");
	#endif

	return res;
}

void EventList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void EventList::paintItem(unsigned int pos)
{
	int color;
	int ypos = y+ theight+0 + pos*fheight;
	string datetime1_str, datetime2_str, duration_str;

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}
	else if (liststart+pos == current_event )
	{
		color = COL_MENUCONTENT+ 1; //COL_MENUCONTENTINACTIVE+ 4;
	}
	else
	{
		color = COL_MENUCONTENT;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

	if(liststart+pos<evtlist.size())
	{
		if ( evtlist[liststart+pos].eventID != 0 )
		{
			char tmpstr[256];
			struct tm *tmStartZeit = localtime(&evtlist[liststart+pos].startTime);


			strftime(tmpstr, sizeof(tmpstr), "date.%a", tmStartZeit );
			datetime1_str = std::string( g_Locale->getText(tmpstr) );

			strftime(tmpstr, sizeof(tmpstr), ". %H:%M, ", tmStartZeit );
			datetime1_str += std::string( tmpstr );

			strftime(tmpstr, sizeof(tmpstr), " %d. ", tmStartZeit );
			datetime2_str = std::string( tmpstr );
			strftime(tmpstr,sizeof(tmpstr), "date.%b", tmStartZeit );
			datetime2_str += std::string( g_Locale->getText(tmpstr) );
			datetime2_str += std::string(".");

        	sprintf(tmpstr, "[%d min]", evtlist[liststart+pos].duration / 60 );
        	duration_str = std::string( tmpstr );
        }

		// 1st line
		g_Fonts->eventlist_datetime->RenderString(x+5,         ypos+ fheight1+3, fwidth1+5,
		        datetime1_str.c_str(), color);
		g_Fonts->eventlist_datetime->RenderString(x+5+fwidth1, ypos+ fheight1+3, width-fwidth1-10- 20,
		        datetime2_str.c_str(), color);

		int seit = ( evtlist[liststart+pos].startTime - time(NULL) ) / 60;
		if ( (seit> 0) && (seit<100) && (duration_str.length()!=0) )
		{
			char beginnt[100];
			sprintf((char*) &beginnt, "in %d min", seit);
			int w= g_Fonts->eventlist_itemSmall->getRenderWidth(beginnt) + 10;

			g_Fonts->eventlist_itemSmall->RenderString(x+width-fwidth2-5- 20- w, ypos+ fheight1+3, fwidth2, beginnt, color);
		}
		g_Fonts->eventlist_itemSmall->RenderString(x+width-fwidth2-5- 20, ypos+ fheight1+3, fwidth2,
		        duration_str.c_str(), color);
		// 2nd line
		g_Fonts->eventlist_itemLarge->RenderString(x+ 20, ypos+ fheight, width- 25- 20,
		        evtlist[liststart+pos].description.c_str(), color);
	}
}

void EventList::paintHead()
{
	char l_name[100];
	snprintf(l_name, sizeof(l_name), g_Locale->getText("epglist.head").c_str(), name.c_str() );

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width, l_name, COL_MENUHEAD);

}

void EventList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	if (evtlist[0].eventID != 0)
		g_FrameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	g_FrameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((evtlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}


