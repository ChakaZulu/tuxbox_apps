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


#include <gui/eventlist.h>

#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino.h>

#include <zapit/client/zapitclient.h> /* CZapitClient::Utf8_to_Latin1 */

#include <algorithm>

// sort operators
bool sortById (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.eventID < b.eventID ;
}
bool sortByDescription (const CChannelEvent& a, const CChannelEvent& b)
{
	if(a.description == b.description)
		return a.eventID < b.eventID;
	else
		return a.description < b.description ;
}

EventList::EventList()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = 0;
	current_event = 0;

	width  = 580;
	//height = 440;
	height = 480;
	theight  = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->getHeight();
	fheight1 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
	{
		int h1 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getHeight();
		int h2 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->getHeight();
		fheight2 = (h1 > h2) ? h1 : h2;
	}
	fheight = fheight1 + fheight2 + 2;
	fwidth1 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->getRenderWidth("DDD, 00:00,  ");
	fwidth2 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth("[999 min] ");


	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
	sort_mode = 0;
}


EventList::~EventList()
{
}


void EventList::readEvents(const t_channel_id channel_id)
{
	current_event = (unsigned int)-1;
	evtlist = g_Sectionsd->getEventsServiceKey(channel_id);
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

		evt.description = CZapitClient::Utf8_to_Latin1(g_Locale->getText("epglist.noevents"));
		evt.eventID = 0;
		evtlist.push_back(evt);

	}
	if (current_event == (unsigned int)-1)
		current_event = 0;
	selected= current_event;

	return;
}


int EventList::exec(const t_channel_id channel_id, const std::string& channelname) // UTF-8
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	name = channelname;
	sort_mode=0;
	paintHead();
	readEvents(channel_id);
	paint();

	int oldselected = selected;

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "epg-Eventlist: %08x \"%s\"", channel_id, channelname.c_str() );
		g_ActionLog->println(buf);
	#endif

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_chanlist );

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_chanlist );

		if (msg == (neutrino_msg_t)g_settings.key_channelList_pageup)
		{
			selected+=listmaxshow;
			if (selected>evtlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (msg == (neutrino_msg_t)g_settings.key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=evtlist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (msg == (neutrino_msg_t)g_settings.key_channelList_sort)
		{
			unsigned long long selected_id = evtlist[selected].eventID;
			if(sort_mode==0)
			{
				sort_mode=1;
				sort(evtlist.begin(),evtlist.end(),sortByDescription);
			}
			else
			{
				sort_mode=0;
				sort(evtlist.begin(),evtlist.end(),sortById);
			}
			// find selected
			for ( selected=0 ; selected < evtlist.size(); selected++ )
			{
				if ( evtlist[selected].eventID == selected_id )
					break;
			}
			oldselected=selected;
			if(selected <=listmaxshow)
				liststart=0;
			else
				liststart=(selected/listmaxshow)*listmaxshow;
			hide();
			paintHead();
			paint();
		}

//  -- I commented out the following part (code is working)
//  -- reason: this is a little bit confusing, because e.g. you can enter the function
//  -- with RED, but pressing RED doesn't leave - it triggers a record timer instead
//  -- I think it's sufficient, to press RIGHT or HELP to get movie details and then
//  -- press "auto record" or "auto switch"  (rasc 2003-06-28)
//  --- hm, no need to comment out that part, leave the decision to the user
//  --- either set addrecord timer key to "no key" and leave eventlist with red (default now),
//  --- or set addrecord timer key to "red key" (zwen 2003-07-29)

		else if (msg == (neutrino_msg_t)g_settings.key_channelList_addrecord)
		{
			if(g_settings.recording_type > 0)
			{
				CTimerdClient timerdclient;
				if(timerdclient.isTimerdAvailable())
				{
					timerdclient.addRecordTimerEvent(channel_id,
																evtlist[selected].startTime,
																evtlist[selected].startTime + evtlist[selected].duration,
																evtlist[selected].eventID, evtlist[selected].startTime,
																evtlist[selected].startTime - (ANNOUNCETIME + 120),
																"", true );
					ShowMsgUTF("timer.eventrecord.title", g_Locale->getText("timer.eventrecord.msg"), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
				}
				else
					printf("timerd not available\n");
			}					
		}
		else if ( msg == (neutrino_msg_t) g_settings.key_channelList_addremind )
	   {
			CTimerdClient timerdclient;
			if(timerdclient.isTimerdAvailable())
			{
				timerdclient.addZaptoTimerEvent(channel_id, 
														  evtlist[selected].startTime,
														  evtlist[selected].startTime - ANNOUNCETIME, 0,
														  evtlist[selected].eventID, evtlist[selected].startTime,
														  "");
				ShowMsgUTF("timer.eventtimed.title", g_Locale->getText("timer.eventtimed.msg"), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
			}
			else
				printf("timerd not available\n");
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
		else if ((msg == CRCInput::RC_timeout                             ) ||
			 (msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
		{
			selected = oldselected;
			loop=false;
		}

		else if ( msg==CRCInput::RC_left || msg==CRCInput::RC_red )
		{
			loop= false;
		}
		else if (msg==CRCInput::RC_help || msg==CRCInput::RC_right || msg==CRCInput::RC_ok)
		{
			if ( evtlist[selected].eventID != 0 )
			{
				hide();

				res = g_EpgData->show(channel_id, evtlist[selected].eventID, &evtlist[selected].startTime);
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
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
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
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void EventList::paintItem(unsigned int pos)
{
	fb_pixel_t color;
	int ypos = y+ theight+0 + pos*fheight;
	std::string datetime1_str, datetime2_str, duration_str;

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}
	else if (liststart+pos == current_event )
	{
		color = COL_MENUCONTENT_PLUS_1;
	}
	else
	{
		color = COL_MENUCONTENT;
	}

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

	if(liststart+pos<evtlist.size())
	{
		if ( evtlist[liststart+pos].eventID != 0 )
		{
			char tmpstr[256];
			struct tm *tmStartZeit = localtime(&evtlist[liststart+pos].startTime);


			strftime(tmpstr, sizeof(tmpstr), "date.%a", tmStartZeit );
			datetime1_str = g_Locale->getText(tmpstr);

			strftime(tmpstr, sizeof(tmpstr), ". %H:%M, ", tmStartZeit );
			datetime1_str += tmpstr;

			strftime(tmpstr, sizeof(tmpstr), " %d. ", tmStartZeit );
			datetime2_str = tmpstr;

			strftime(tmpstr,sizeof(tmpstr), "date.%b", tmStartZeit );
			datetime2_str += g_Locale->getText(tmpstr);

			datetime2_str += '.';

			sprintf(tmpstr, "[%d min]", evtlist[liststart+pos].duration / 60 );
			duration_str = tmpstr;
		}

		// 1st line
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->RenderString(x+5,         ypos+ fheight1+3, fwidth1+5,            datetime1_str, color, 0, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->RenderString(x+5+fwidth1, ypos+ fheight1+3, width-fwidth1-10- 20, datetime2_str, color, 0, true); // UTF-8

		int seit = ( evtlist[liststart+pos].startTime - time(NULL) ) / 60;
		if ( (seit> 0) && (seit<100) && (duration_str.length()!=0) )
		{
			char beginnt[100];
			sprintf((char*) &beginnt, "in %d min", seit);
			int w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth(beginnt) + 10;

			g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString(x+width-fwidth2-5- 20- w, ypos+ fheight1+3, fwidth2, beginnt, color);
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString(x+width-fwidth2-5- 20, ypos+ fheight1+3, fwidth2, duration_str, color, 0, true); // UTF-8
		// 2nd line
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->RenderString(x+ 20, ypos+ fheight, width- 25- 20, evtlist[liststart+pos].description, color);
	}
}

void EventList::paintHead()
{
	char l_name[100];
	snprintf(l_name, sizeof(l_name), g_Locale->getText("epglist.head"), name.c_str()); // UTF-8

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->RenderString(x+10,y+theight+1, width, l_name, COL_MENUHEAD, 0, true); // UTF-8
}

void EventList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	if (evtlist[0].eventID != 0)
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ width- 30, y+ 5 );

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((evtlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3);
}






//
//  -- EventList Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
// 

int CEventListHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	EventList     *e;
	CChannelList  *channelList;


	if (parent) {
		parent->hide();
	}

	e = new EventList;

	channelList = CNeutrinoApp::getInstance()->channelList;
	e->exec(channelList->getActiveChannel_ChannelID(), channelList->getActiveChannelName()); // UTF-8
	delete e;

	return res;
}


