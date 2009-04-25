/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: channellist.cpp,v 1.210 2009/04/25 14:50:34 rhabarber1848 Exp $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2007-2009 Stefan Seyfried

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/channellist.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/progressbar.h>

#include <system/settings.h>
#include <system/lastchannel.h>


#include <gui/bouquetlist.h>
#include <daemonc/remotecontrol.h>
#include <zapit/client/zapittools.h>

extern CBouquetList * bouquetList;       /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
extern CZapitClient::SatelliteList satList;
int info_height = 0;
#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_stop();
#endif

CChannelList::CChannel::CChannel(const int _key, const int _number, const std::string& _name, const t_satellite_position _satellitePosition, const t_channel_id ids)
{
	key                 = _key;
	number              = _number;
	name                = _name;
	satellitePosition   = _satellitePosition;
	channel_id          = ids;
	bAlwaysLocked       = false;
	last_unlocked_EPGid = 0;
}


CChannelList::CChannelList(const char * const Name, bool hMode)
{
	frameBuffer = CFrameBuffer::getInstance();
	name = Name;
	selected = 0;
	// width = 560;
	// height = 420 + (1 + 3 + 16 + 3);

	liststart = 0;
	tuned=0xfffffff;
	zapProtection = NULL;
	this->historyMode = hMode;
}

CChannelList::~CChannelList()
{
	for (std::vector<CChannel *>::iterator it = chanlist.begin(); it != chanlist.end(); it++)
	{
		delete (*it);
	}
	chanlist.clear();
}

int CChannelList::exec()
{
	int nNewChannel = show();

	if ( nNewChannel > -1)
	{
		zapTo(nNewChannel);
		return menu_return::RETURN_REPAINT;
	}
	else if ( nNewChannel == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}
}

void CChannelList::updateEvents(void)
{
	time_t atime = time(NULL);
	int cnt=0;

	if (displayNext) {
		if (listmaxshow) {
			for (uint count=0; (count<listmaxshow) && (liststart+count<chanlist.size()); count++){
				// search only for channels whose current event is over
				if (1 || /*(chanlist[liststart+count]->nextEvent.text.length() == 0) ||*/
					((long)(chanlist[liststart+count]->nextEvent.startTime) < atime))
				{
					CChannelEventList events = g_Sectionsd->getEventsServiceKey(chanlist[liststart+count]->channel_id);
					chanlist[liststart+count]->nextEvent.startTime = (long)0x7fffffff;
					for ( CChannelEventList::iterator e= events.begin(); e != events.end(); ++e ) {
						if (((long)(e->startTime) > atime) && 
							((e->startTime) < (long)(chanlist[liststart+count]->nextEvent.startTime)))
						{
							chanlist[liststart+count]->nextEvent= *e;
						}
					}
				}
			}
		}
	} else {
		t_channel_id *p_requested_channels = NULL;
		int size_requested_channels = 0;

		if (listmaxshow) {
			size_requested_channels = listmaxshow*sizeof(t_channel_id);
			p_requested_channels 	= (t_channel_id*)malloc(size_requested_channels);
			if (p_requested_channels != NULL) {
				for (uint count=0; (count<listmaxshow) && (liststart+count<chanlist.size()); count++){
					// search only for channels whose current event is over
					if ((chanlist[liststart+count]->currentEvent.text.length() == 0) ||
						((long)(chanlist[liststart+count]->currentEvent.startTime + chanlist[liststart+count]->currentEvent.duration) < atime))
					{
						chanlist[liststart+count]->currentEvent = CChannelEvent(); // clear old event
						p_requested_channels[cnt++] = chanlist[liststart+count]->channel_id;
					}
				}
			}
		}
		size_requested_channels = cnt * sizeof(t_channel_id); // update to real size

		if (size_requested_channels) {
			/* request tv channel list if current mode is not radio mode */
			/* request only the events of the channel of the list */
			CChannelEventList events = g_Sectionsd->getChannelEvents((CNeutrinoApp::getInstance()->getMode()) != NeutrinoMessages::mode_radio, p_requested_channels, size_requested_channels);

			for ( CChannelEventList::iterator e= events.begin(); e != events.end(); ++e ) {
				for (uint count=0; (count<listmaxshow) && (liststart+count<chanlist.size()); count++){
					if (chanlist[liststart+count]->channel_id == e->get_channel_id())
					{
						chanlist[liststart+count]->currentEvent= *e;
						break;
					}
				}
			}
		}
		if (p_requested_channels != NULL) free(p_requested_channels);
	}
}


void CChannelList::addChannel(int key, int number, const std::string& _name, const t_satellite_position satellitePosition, t_channel_id ids)
{
	chanlist.push_back(new CChannel(key, number, _name, satellitePosition, ids));
}

void CChannelList::addChannel(CChannelList::CChannel* chan)
{
	if (chan != NULL)
		chanlist.push_back(chan);
}

CChannelList::CChannel* CChannelList::getChannel( int number)
{
	for (uint i=0; i< chanlist.size();i++)
	{
		if (chanlist[i]->number == number)
			return chanlist[i];
	}
	return(NULL);
}

int CChannelList::getKey(int id)
{
	return chanlist[id]->key;
}

static const std::string empty_string;

const std::string & CChannelList::getActiveChannelName(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->name;
	else
		return empty_string;
}

t_satellite_position CChannelList::getActiveSatellitePosition(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->satellitePosition;
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_ChannelID(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->channel_id;
	else
		return 0;
}

int CChannelList::getActiveChannelNumber(void) const
{
	return (selected + 1);
}

int CChannelList::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	width  = w_max (590, 0);
	height = h_max (420 + (1+3+16+3), 60);


	if (chanlist.empty())
	{
		//evtl. anzeige dass keine kanalliste....
		return res;
	}
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, name.c_str());

	int buttonHeight = 7 + std::min(16, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight());
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
	listmaxshow = (height - theight - buttonHeight -0)/fheight;
	height = theight + buttonHeight + listmaxshow * fheight;
	info_height = fheight + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getHeight() + 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;

	displayNext = false;
	paintHead();
//	updateEvents();
	paint();

	int oldselected = selected;
	int zapOnExit = false;
	bool bShowBouquetList = false;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
		{
			selected = oldselected;
			loop=false;
		}
		else if (msg_repeatok == CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			int step = 0;
			int prev_selected = selected;

			step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
			selected -= step;
			if((prev_selected-step) < 0)		// because of uint
				selected = chanlist.size() - 1;

			paintItem(prev_selected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
				paint();
			else
				paintItem(selected - liststart);
				
			paintHead();
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			int step = 0;
			int prev_selected = selected;

			step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			selected += step;

			if(selected >= chanlist.size())
				selected = ((step==listmaxshow)&&(selected<(((chanlist.size()/listmaxshow)+1)*listmaxshow)))?(chanlist.size()-1):0;

			paintItem(prev_selected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
				paint();
			else
				paintItem(selected - liststart);
				
			paintHead();
		}
		else if (msg_repeatok == g_settings.key_bouquet_up && bouquetList != NULL)
		{
			if (bouquetList->Bouquets.size() > 0)
			{
				int nNext = (bouquetList->getActiveBouquetNumber()+1) % bouquetList->Bouquets.size();
				bouquetList->activateBouquet( nNext );
				res = bouquetList->showChannelList();
				loop = false;
			}
		}
		else if (msg_repeatok == g_settings.key_bouquet_down && bouquetList != NULL)
		{
			if (bouquetList->Bouquets.size() > 0)
			{
				int nNext = (bouquetList->getActiveBouquetNumber()+bouquetList->Bouquets.size()-1) % bouquetList->Bouquets.size();
				bouquetList->activateBouquet(nNext);
				res = bouquetList->showChannelList();
				loop = false;
			}
		}
		else if ( msg == CRCInput::RC_ok )
		{
			zapOnExit = true;
			loop=false;
		}
		else if ( ( msg == CRCInput::RC_setup ) &&
				  ( bouquetList != NULL ) )
		{
			bShowBouquetList = true;
			loop=false;
		}
		else if (this->historyMode && CRCInput::isNumeric(msg))
		{ //numeric zap
			switch (msg)
			{
				case CRCInput::RC_0:selected = 0; break;
				case CRCInput::RC_1:selected = 1; break;
				case CRCInput::RC_2:selected = 2; break;
				case CRCInput::RC_3:selected = 3; break;
				case CRCInput::RC_4:selected = 4; break;
				case CRCInput::RC_5:selected = 5; break;
				case CRCInput::RC_6:selected = 6; break;
				case CRCInput::RC_7:selected = 7; break;
				case CRCInput::RC_8:selected = 8; break;
				case CRCInput::RC_9:selected = 9; break;
			};
			zapOnExit = true;
			loop = false;
		}
		else if( (msg==CRCInput::RC_green) ||
			 (msg==CRCInput::RC_yellow) ||
//			 (msg==CRCInput::RC_blue) ||
			 (CRCInput::isNumeric(msg)) )
		{
			//pushback key if...
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else if ( msg == CRCInput::RC_red )   // changed HELP by RED (more straight forward) [rasc 28.06.2003]
		{
			hide();

			if ( g_EventList->exec(chanlist[selected]->channel_id, chanlist[selected]->name) == menu_return::RETURN_EXIT_ALL) // UTF-8
			{
				res = -2;
				loop = false;
			}
//			else
//			{
//				g_RCInput->getMsg( &msg, &data, 0 );
//
//				if ( ( msg != CRCInput::RC_red ) &&
//				     ( msg != CRCInput::RC_timeout ) )
//				{
//					// RC_red schlucken
//					g_RCInput->postMsg( msg, data );
//				}
//
//			}
			paintHead();
			paint();

		}
		else if ( msg == CRCInput::RC_blue )
		{
			displayNext = !displayNext;
			paintHead(); // update button bar
			paint();
		}
		else if ( msg == CRCInput::RC_help )
		{
			CChannelEvent *p_event=NULL;

			hide();

			if (displayNext)
			{
				p_event = &(chanlist[selected]->nextEvent);
			}

			if(p_event && p_event->eventID)
			{
				g_EpgData->show(chanlist[selected]->channel_id,p_event->eventID,&(p_event->startTime));
			}
			else
			{
				g_EpgData->show(chanlist[selected]->channel_id);
			}

		paintHead();
		paint();

		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = - 2;
			}
		}
	}
	hide();

	if (bShowBouquetList)
	{
		if ( bouquetList->exec( true ) == menu_return::RETURN_EXIT_ALL )
			res = -2;
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

	if(zapOnExit)
	{
		return(selected);
	}
	else
	{
		return(res);
	}

}

void CChannelList::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height+ info_height+ 5);
	clearItem2DetailsLine ();
	//displayNext = 0; // always start with current events
}

bool CChannelList::showInfo(int pos, int epgpos)
{
	if((pos >= (signed int) chanlist.size()) || (pos<0))
	{
		return false;
	}

	CChannel* chan = chanlist[pos];
	g_InfoViewer->showTitle(pos+1, chan->name, chan->satellitePosition, chan->channel_id, true, epgpos); // UTF-8
	return true;
}

int CChannelList::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ( msg == NeutrinoMessages::EVT_PROGRAMLOCKSTATUS)
	{
		// 0x100 als FSK-Status zeigt an, dass (noch) kein EPG zu einem Kanal der NICHT angezeigt
		// werden sollte (vorgesperrt) da ist
		// oder das bouquet des Kanals ist vorgesperrt

		//printf("program-lock-status: %d\n", data);

		if ((g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) || (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED))
		{
			if ( zapProtection != NULL )
				zapProtection->fsk = data;
			else
			{
				// require password if either
				// CHANGETOLOCK mode and channel/bouquet is pre locked (0x100)
				// ONSIGNAL mode and fsk(data) is beyond configured value
				// if programm has already been unlocked, dont require pin
				if ((data >= (neutrino_msg_data_t)g_settings.parentallock_lockage) &&
					 ((chanlist[selected]->last_unlocked_EPGid != g_RemoteControl->current_EPGid) || (g_RemoteControl->current_EPGid == 0)) &&
					 ((g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_CHANGETOLOCKED) || (data >= 0x100)))
				{
					g_RemoteControl->stopvideo();
					zapProtection = new CZapProtection( g_settings.parentallock_pincode, data );

					if ( zapProtection->check() )
					{
						g_RemoteControl->startvideo();

						// remember it for the next time
						chanlist[selected]->last_unlocked_EPGid= g_RemoteControl->current_EPGid;
					}
					delete zapProtection;
					zapProtection = NULL;
				}
				else
					g_RemoteControl->startvideo();
			}
		}
		else
			g_RemoteControl->startvideo();

		return messages_return::handled;
	}
    else
		return messages_return::unhandled;
}


//
// -- Zap to channel with channel_id
//
bool CChannelList::zapTo_ChannelID(const t_channel_id channel_id)
{
	for (unsigned int i=0; i<chanlist.size(); i++) {
		if (chanlist[i]->channel_id == channel_id) {
			zapTo (i);
			return true;
		}
	}

    return false;
}

bool CChannelList::adjustToChannelID(const t_channel_id channel_id)
{
	unsigned int i;

	for (i=0; i<chanlist.size(); i++) {
		if (chanlist[i]->channel_id == channel_id)
		{
			selected= i;
//			CChannel* chan = chanlist[selected];
			lastChList.store (selected, channel_id, false);

			tuned = i;
			if (bouquetList != NULL)
				bouquetList->adjustToChannel( getActiveChannelNumber());
			return true;
		}
	}
	return false;
}

void CChannelList::zapTo(int pos, bool forceStoreToLastChannels)
{
	if (chanlist.empty())
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_CHANNELLIST_NONEFOUND)); // UTF-8
		return;
	}
	if ( (pos >= (signed int) chanlist.size()) || (pos< 0) )
	{
		pos = 0;
	}

	selected= pos;
	CChannel* chan = chanlist[selected];
	lastChList.store (selected, chan->channel_id, forceStoreToLastChannels);

	if ( pos!=(int)tuned )
	{
#ifndef TUXTXT_CFG_STANDALONE
		if(g_settings.tuxtxt_cache && !CNeutrinoApp::getInstance ()->recordingstatus)
		{
			tuxtxt_stop();
		}
#endif
		tuned = pos;
		if (g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGMODE] & 0x02)
		{	/* microoptimization: only poll sectionsd if epg title display is configured
			   not sure if this is necessary, but the extra check won't hurt... */
			CSectionsdClient::CurrentNextInfo info;
			g_Sectionsd->getCurrentNextServiceKey(chan->channel_id, info);
			CLCD::getInstance()->setEPGTitle(info.current_name);
		}
		CLCD::getInstance()->showServicename(chan->name);
		g_RemoteControl->zapTo_ChannelID(chan->channel_id, chan->name, !chan->bAlwaysLocked); // UTF-8
	}
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	if (bouquetList != NULL)
		bouquetList->adjustToChannel( getActiveChannelNumber());

	/* zapTo can take some time.
	   To prevent unwanted "multizaps", clear the RC buffer" */
	g_RCInput->clearRCMsg();
}



int CChannelList::numericZap(neutrino_msg_t key)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	if (chanlist.empty()) {
		DisplayErrorMessage(g_Locale->getText(LOCALE_CHANNELLIST_NONEFOUND)); // UTF-8
		return res;
	}


	// -- quickzap "0" to last seen channel...
	// -- (remains for those who want to avoid the channel history menue)
	// -- (--> girl friend complained about the history menue, so be it...)
	// -- we should be able to configure this in the future, so "0"
	// -- will do quizap or history...
	if (key == g_settings.key_lastchannel) {
		int  ch;

		if( (ch=lastChList.getlast(1)) != -1)
		{
			if ((unsigned int)ch != tuned)
			{
				//printf("quicknumtune(0)\n");
				lastChList.clear_storedelay (); // ignore store delay
				zapTo(ch);		        // zap to last
			}
		}
		return res;
	}

	// -- zap history bouquet, similar to "0" quickzap,
	// -- but shows a menue of last channels
	if (key == g_settings.key_zaphistory) {

		if (this->lastChList.size() > 1) {
			CChannelList channelList("Channel history", true);

			for ( unsigned int i = 1 ; i < this->lastChList.size() ; ++i) {
				int channelnr = this->lastChList.getlast(i);
				if (channelnr < int(this->chanlist.size())) {
					CChannel* channel = new CChannel(*this->chanlist[channelnr]);
					channelList.addChannel(channel);
        			}
			}

			if (channelList.getSize() != 0) {
				this->frameBuffer->paintBackground();
				int newChannel = channelList.show() ;

				if (newChannel > -1) {
					int lastChannel(this->lastChList.getlast(newChannel + 1));
					if (lastChannel > -1) this->zapTo(lastChannel, true);
				}
			}
		}
		return res;
	}

	int ox=300;
	int oy=200;
	int sx = 4 * g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(widest_number) + 14;
	int sy = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight() + 6;
	char valstr[10];
	int chn = CRCInput::getNumericValue(key);
	int pos = 1;
	int lastchan= -1;
	bool doZap = true;
	bool showEPG = false;

	while(1)
	{
		if (lastchan != chn)
		{
			sprintf((char*) &valstr, "%d", chn);
			while(strlen(valstr)<4)
				strcat(valstr,"\xB7");   //MIDDLE DOT 

			frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR_PLUS_0, RADIUS_MID);

			for (int i=3; i>=0; i--)
			{
				valstr[i+ 1]= 0;
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(ox+7+ i*((sx-14)>>2), oy+sy-3, sx, &valstr[i], COL_INFOBAR);
			}

			showInfo(chn - 1);
			lastchan= chn;
		}

		g_RCInput->getMsg( &msg, &data, g_settings.timing[SNeutrinoSettings::TIMING_NUMERICZAP] * 10 );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg == CRCInput::RC_timeout )
		{
			if ( ( chn > (int)chanlist.size() ) || (chn == 0) )
				chn = tuned + 1;
			break;
		}
		else if (CRCInput::isNumeric(msg))
		{
			if (pos == 4)
			{
				chn = 0;
				pos = 1;
			}
			else
			{
				chn *= 10;
				pos++;
			}
			chn += CRCInput::getNumericValue(msg);
		}
		else if ( msg == CRCInput::RC_ok )
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
			{
				chn = tuned + 1;
			}
			break;
		}
		else if (msg_repeatok == g_settings.key_quickzap_down)
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else
			{
				chn--;

				if (chn > (int)chanlist.size())
					chn = (int)chanlist.size();
			}
		}
		else if (msg_repeatok == g_settings.key_quickzap_up)
		{
			chn++;

			if (chn > (int)chanlist.size())
				chn = 1;
		}
		else if ( ( msg == CRCInput::RC_home ) ||
				  ( msg == CRCInput::RC_left ) ||
				  ( msg == CRCInput::RC_right) )
		{
			// Abbruch ohne Channel zu wechseln
			doZap = false;
			break;
		}
		else if ( msg == CRCInput::RC_red )
		{
			// Rote Taste zeigt EPG fuer gewaehlten Kanal an
			if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) )
			{
				doZap = false;
				showEPG = true;
				break;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			doZap = false;
			res = menu_return::RETURN_EXIT_ALL;
			break;
		}
	}

	frameBuffer->paintBackgroundBoxRel(ox, oy, sx, sy);

	chn--;
	if (chn<0)
		chn=0;
	if ( doZap )
	{
		zapTo( chn );
	}
	else
	{
		showInfo(tuned);
		g_InfoViewer->killTitle();

		// Rote Taste zeigt EPG fuer gewaehlten Kanal an
		if ( showEPG )
			g_EventList->exec(chanlist[chn]->channel_id, chanlist[chn]->name);
	}

	return res;
}

void CChannelList::virtual_zap_mode(bool up)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	if (chanlist.empty()) {
		DisplayErrorMessage(g_Locale->getText(LOCALE_CHANNELLIST_NONEFOUND)); // UTF-8
		return;
	}

	int chn = getActiveChannelNumber() + (up ? 1 : -1);
	if (chn > (int)chanlist.size())
	  chn = 1;
	if (chn == 0)
	  chn = (int)chanlist.size();
	int lastchan= -1;
	bool doZap = true;
	bool showEPG = false;
	int epgpos = 0;

	while(1)
	{
		if ((lastchan != chn) || (epgpos != 0))
		{
			showInfo(chn- 1, epgpos);
			lastchan= chn;
		}
		epgpos = 0;
		g_RCInput->getMsg( &msg, &data, atoi(g_settings.timing_string[3])*10 ); // virtual zap timout = TIMING_INFOBAR
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;
		//printf("########### %u ### %u #### %u #######\n", msg, NeutrinoMessages::EVT_TIMER, CRCInput::RC_timeout);

		if ( msg == CRCInput::RC_ok )
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
			{
				chn = tuned + 1;
			}
			break;
		}
		else if (msg_repeatok == CRCInput::RC_left)
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else
			{
				chn--;

				if (chn > (int)chanlist.size())
					chn = (int)chanlist.size();
			}
		}
		else if (msg_repeatok == CRCInput::RC_right)
		{
			chn++;

			if (chn > (int)chanlist.size())
				chn = 1;
		}
		else if (msg_repeatok == CRCInput::RC_up)
		{
			epgpos = -1;
		}
		else if (msg_repeatok == CRCInput::RC_down)
		{
			epgpos = 1;
		}
		else if ( ( msg == CRCInput::RC_home ) || ( msg == CRCInput::RC_timeout ) )
		{
			// Abbruch ohne Channel zu wechseln
			doZap = false;
			break;
		}
		else if ( msg == CRCInput::RC_red )
		{
			// Rote Taste zeigt EPG fuer gewaehlten Kanal an
			if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) )
			{
				doZap = false;
				showEPG = true;
				break;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			doZap = false;
			break;
		}
	}
	g_InfoViewer->clearVirtualZapMode();

	chn--;
	if (chn<0)
		chn=0;
	if ( doZap )
	{
		zapTo( chn );
	}
	else
	{
		showInfo(tuned);
		g_InfoViewer->killTitle();

		// Rote Taste zeigt EPG fuer gewaehlten Kanal an
		if ( showEPG )
			g_EventList->exec(chanlist[chn]->channel_id, chanlist[chn]->name);
	}
}

void CChannelList::quickZap(neutrino_msg_t key)
{
        if(chanlist.size()==0)
        {
                //evtl. anzeige dass keine kanalliste....
                return;
        }

        if (key == g_settings.key_quickzap_down)
        {
                if(selected==0)
                        selected = chanlist.size()-1;
                else
                        selected--;
                //                              CChannel* chan = chanlist[selected];
        }
        else if (key == g_settings.key_quickzap_up)
        {
                selected = (selected+1)%chanlist.size();
                //                      CChannel* chan = chanlist[selected];
        }

        zapTo( selected );
}

int CChannelList::hasChannel(int nChannelNr)
{
	for (uint i=0;i<chanlist.size();i++)
	{
		if (getKey(i) == nChannelNr)
			return(i);
	}
	return(-1);
}

// for adjusting bouquet's channel list after numzap or quickzap
void CChannelList::setSelected( int nChannelNr)
{
	selected = nChannelNr;
}

void CChannelList::paintDetails(unsigned int index)
{
	CChannelEvent *p_event;
	if (displayNext) {
		p_event = &chanlist[index]->nextEvent;
	} else {
		p_event = &chanlist[index]->currentEvent;
	}

	if (index >= chanlist.size() || p_event->description.empty())
	{
		frameBuffer->paintBackgroundBoxRel(x, y+ height, width, info_height);
	}
	else
	{
		frameBuffer->paintHLineRel(x, width, y + height, COL_INFOBAR_SHADOW_PLUS_0);
		frameBuffer->paintBoxRel(x, y + height + 1, width, info_height - 1, COL_MENUCONTENTDARK_PLUS_0);

		char cNoch[50]; // UTF-8
		char cSeit[50]; // UTF-8

		struct		tm *pStartZeit = localtime(&p_event->startTime);
		unsigned 	seit = ( time(NULL) - p_event->startTime ) / 60;

		if (displayNext) {
			sprintf(cNoch, "(%d min)", p_event->duration / 60);
			sprintf(cSeit, g_Locale->getText(LOCALE_CHANNELLIST_START), pStartZeit->tm_hour, pStartZeit->tm_min);
		} else {
			sprintf(cSeit, g_Locale->getText(LOCALE_CHANNELLIST_SINCE), pStartZeit->tm_hour, pStartZeit->tm_min);
			int noch = (p_event->startTime + p_event->duration - time(NULL)) / 60;
			if ((noch< 0) || (noch>=10000))
				noch= 0;
			sprintf(cNoch, "(%d / %d min)", seit, noch);
		}
		int seit_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(cSeit, true); // UTF-8
		int noch_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(cNoch, true); // UTF-8

		std::string text1= p_event->description;
		std::string text2= p_event->text;

		int xstart = 10;
		if (g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text1) > (width - 30 - seit_len) )
		{
			// zu breit, Umbruch versuchen...
			int pos;
			do
			{
				pos = text1.find_last_of("[ -.]+");
				if ( pos!=-1 )
					text1 = text1.substr( 0, pos );
			} while ( ( pos != -1 ) && (g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text1) > (width - 30 - seit_len) ) );

			std::string text3 = p_event->description.substr(text1.length()+ 1);
			if (!(text2.empty()))
				text3 += "\xB7"; //MIDDLE DOT

			xstart += g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text3);
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ 2* fheight, width - 30- noch_len, text3, COL_MENUCONTENTDARK);
		}

		if (!(text2.empty()))
		{
			while ( text2.find_first_of("[ -.+*#?=!$%&/]+") == 0 )
				text2 = text2.substr( 1 );
			text2 = text2.substr( 0, text2.find('\n') );
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x+ xstart, y+ height+ 5+ 2* fheight, width- xstart- 20- noch_len, text2, COL_MENUCONTENTDARK);
		}

		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ fheight, width - 30 - seit_len, text1, COL_MENUCONTENTDARK);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString (x+ width- 10- seit_len, y+ height+ 5+    fheight   , seit_len, cSeit, COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x+ width- 10- noch_len, y+ height+ 5+ 2* fheight- 2, noch_len, cNoch, COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
}


//
// -- Decoreline to connect ChannelDisplayLine with ChannelDetail display
// -- 2002-03-17 rasc
//

void CChannelList::clearItem2DetailsLine ()
{
	paintItem2DetailsLine (-1, 0);
}

void CChannelList::paintItem2DetailsLine (int pos,unsigned  int ch_index)
{
	#define ConnectLineBox_Width	16
	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + theight+0 + pos*fheight;
	int ypos2 = y + height;
	int ypos1a = ypos1 + (fheight/2)-2;
	int ypos2a = ypos2 + (info_height/2)-2;
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;
	int c_rad_small = RADIUS_SMALL;

	CChannelEvent *p_event;
	if (displayNext) {
		p_event = &chanlist[ch_index]->nextEvent;
	} else {
		p_event = &chanlist[ch_index]->currentEvent;
	}

	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos,y, ConnectLineBox_Width, height+info_height);

	// paint Line if detail info (and not valid list pos)
	if (pos >= 0 &&  ch_index < chanlist.size() && p_event->description != "")
	{
		// 1. col thick line
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 4,fheight, col2, c_rad_small, CORNER_LEFT); // item marker
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-3, ypos1, 8,fheight, col1, c_rad_small, CORNER_LEFT); 
	
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 4,info_height, col1);

		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 4,ypos2a-ypos1a, col1);

		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,4, col1);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos2a, 12,4, col1);

		// 2. col small line		
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 1,info_height, col2);

		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 1,ypos2a-ypos1a+4, col2);

		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,1, col2);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-12, ypos2a, 8,1, col2);
		
		// -- small Frame around infobox
		frameBuffer->paintBoxRel(x,         ypos2, 			2,		info_height, 	col1);
		frameBuffer->paintBoxRel(x+width-2, ypos2, 			2,		info_height, 	col1);
		frameBuffer->paintBoxRel(x        , ypos2, 			width-2,	2,     		col1);
		frameBuffer->paintBoxRel(x        , ypos2+info_height-2, 	width-2,	2, 		col1);
	}
}

void CChannelList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int c_rad_small;

	uint8_t    color;
	fb_pixel_t bgcolor;
	if (liststart + pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		paintDetails(liststart+pos);
		paintItem2DetailsLine (pos, liststart+pos);
		c_rad_small = RADIUS_SMALL;
	}
	else
	{
		color   = !displayNext ? COL_MENUCONTENT : COL_MENUCONTENTINACTIVE;
		bgcolor = !displayNext ? COL_MENUCONTENT_PLUS_0 : COL_MENUCONTENTINACTIVE_PLUS_0;
		c_rad_small = 0;
	}

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor, c_rad_small);
	if(liststart+pos<chanlist.size())
	{
		CChannel* chan = chanlist[liststart+pos];
		//number
		char tmp[10];
		int prg_offset=0;
		int title_offset=0;
		uint8_t tcolor=(liststart + pos == selected) ? color : COL_MENUCONTENT;
		int xtheight=fheight-2;
		
		if(g_settings.channellist_extended)
		{
			prg_offset=42;
			title_offset=6;
		}
		
		sprintf((char*) tmp, "%d", this->historyMode ? pos:CNeutrinoApp::getInstance ()->recordingstatus ? liststart+pos+1 : chan->number);

		CChannelEvent *p_event;
		if (displayNext) {
			p_event = &chan->nextEvent;		
		} else {
			p_event = &chan->currentEvent;
		}

		if (liststart+pos==selected)
		{
			CLCD::getInstance()->showMenuText(0, chan->name.c_str(), -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, p_event->description.c_str());
		}

		int numpos = x+5+numwidth- g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);

		char nameAndDescription[100];
		if (this->historyMode)
			snprintf(nameAndDescription, sizeof(nameAndDescription), ": %d %s", chan->number, ZapitTools::UTF8_to_Latin1(chan->name.c_str()).c_str());
		else
			snprintf(nameAndDescription, sizeof(nameAndDescription), "%s", ZapitTools::UTF8_to_Latin1(chan->name.c_str()).c_str());

		CProgressBar pb;
		int pb_space = prg_offset - title_offset;
		int pb_max = pb_space - 4;
		
		if (!(chan->currentEvent.description.empty()))
		{
			// add MIDDLE DOT separator between name and description
			const char *sep= g_settings.channellist_epgtext_align_right ? "   " :  " \xB7 " ;
			strncat(nameAndDescription, sep, sizeof(nameAndDescription) - (strlen(nameAndDescription) + 1));
			int ch_name_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(nameAndDescription);
			int ch_desc_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(p_event->description);

			if ((width - (int)numwidth - 20 - 15 - prg_offset - ch_name_len) < ch_desc_len)
				ch_desc_len = (width - numwidth - 20 - 15 - ch_name_len - prg_offset);
			if (ch_desc_len < 0)
				ch_desc_len = 0;

			if(g_settings.channellist_extended)
			{		
				if(displayNext)
				{
					struct		tm *pStartZeit = localtime(&p_event->startTime);
			
					sprintf((char*) tmp, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + 5 + numwidth + 6, ypos + xtheight, width - numwidth - 20 - 15 - prg_offset, tmp, tcolor, 0, true);
				}
				else
				{
					time_t jetzt=time(NULL);
					int runningPercent = 0;
							
					if (((jetzt - p_event->startTime + 30) / 60) < 0 )
					{
						runningPercent= 0;
					}
					else
					{
						runningPercent=(jetzt-p_event->startTime) * pb_max / p_event->duration;
						if (runningPercent > pb_max)	// this would lead to negative value in paintBoxRel
							runningPercent = pb_max;	// later on which can be fatal...
					}
					
					// progressbar colors
					int pb_activeCol , pb_passiveCol ;
					if (liststart + pos != selected){
						pb_activeCol = COL_MENUCONTENT_PLUS_3;
						pb_passiveCol = COL_MENUCONTENT_PLUS_1;
					}
					else {
						pb_activeCol = COL_MENUCONTENTSELECTED_PLUS_2;
						pb_passiveCol = COL_MENUCONTENTSELECTED_PLUS_0;
					}
					// progressbar 
					pb.paintProgressBar(x+5+numwidth + title_offset, ypos + fheight/4, pb_space + 2, fheight/2, runningPercent, pb_max, pb_activeCol, pb_passiveCol, pb_activeCol);
				}
			}

			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 5 + numwidth + 10 +prg_offset, ypos + fheight, width - numwidth - 20 - 15 - prg_offset, nameAndDescription, color);

			if (g_settings.channellist_epgtext_align_right){
				// align right
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x + width - 20 - ch_desc_len, ypos + fheight, ch_desc_len, p_event->description, color);
			}
			else{
				// align left
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x + numwidth + 10 + ch_name_len + 5 + prg_offset, ypos + fheight, ch_desc_len, p_event->description, color);
			}
			
		}
		else
			// progressbar with diagonal zero line
			if(g_settings.channellist_extended)
			{
				int pbz_activeCol , pbz_passiveCol ;
				if (liststart + pos != selected){
					pbz_activeCol = COL_MENUCONTENT_PLUS_1;
					pbz_passiveCol = COL_MENUCONTENT_PLUS_0;
				}
				else {
					pbz_activeCol = COL_MENUCONTENTSELECTED_PLUS_2;
					pbz_passiveCol = COL_MENUCONTENTSELECTED_PLUS_0;
				}
				pb.paintProgressBar(x+5+numwidth + title_offset, ypos + fheight/4, pb_space + 2, fheight/2, 0, pb_max, pbz_activeCol, pbz_passiveCol, pbz_activeCol, 0, NULL, 0, NULL, true);
			}
			// name
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 5 + numwidth + 10 + prg_offset, ypos + fheight, width- numwidth - 20 - 15 - prg_offset, nameAndDescription, color);
	}
}

void CChannelList::paintHead()
{
	int timestr_len = 0;
	int provstr_len = 0;
	char *timestr = new char[10];
	char *provstr = new char[20];
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);

	bool gotTime = g_Sectionsd->getIsTimeSet();

	if(gotTime){
		strftime(timestr, 10, "%H:%M", tm);
		timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(timestr, true); // UTF-8
	}
	
	if (g_info.delivery_system == 1)
	{
		for (CZapitClient::SatelliteList::const_iterator satList_it = satList.begin(); satList_it != satList.end(); satList_it++)
			if (satList_it->satPosition == getActiveSatellitePosition())
			{
				sprintf(provstr,"%s", satList_it->satName);
			break;
			}
	}
	else
	{
		CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();
		sprintf(provstr,"%s",CNeutrinoApp::getInstance()->getScanSettings().satOfDiseqc(si.diseqc));
	}
	provstr_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(provstr, true); // UTF-8

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight+0, width-10-timestr_len-10, name, COL_MENUHEAD, 0, true); // UTF-8
	
	g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+width-10-timestr_len-10-provstr_len-10,y+fheight+5, provstr_len+1, provstr, COL_MENUHEAD, 0, true); // UTF-8

	if (gotTime){
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+width-10-timestr_len, y+theight+0, timestr_len+1, timestr, COL_MENUHEAD, 0, true); // UTF-8
	}

	delete[] timestr;
	delete[] provstr;
	paintFoot();
}

struct button_label CChannelListButtons[] =
{
	{ NEUTRINO_ICON_BUTTON_RED, LOCALE_INFOVIEWER_EVENTLIST},
	{ NEUTRINO_ICON_BUTTON_BLUE, LOCALE_INFOVIEWER_NEXT},
	{ NEUTRINO_ICON_BUTTON_HELP_SMALL, LOCALE_EPGMENU_EVENTINFO},
	{ "", LOCALE_GENERIC_EMPTY } // icon for bouquet list button
};

void CChannelList::paintFoot()
{
	int ButtonWidth = width/4;
	int buttonHeight = 7 + std::min(16, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight());
	frameBuffer->paintHLineRel(x, width, y + (height - buttonHeight), COL_INFOBAR_SHADOW_PLUS_0);
	
	if (displayNext) {
		CChannelListButtons[1].locale = LOCALE_INFOVIEWER_NOW;
	} else {
		CChannelListButtons[1].locale = LOCALE_INFOVIEWER_NEXT;
	}
	
	if (bouquetList != NULL) {
		CChannelListButtons[3].locale = LOCALE_BOUQUETLIST_HEAD; 
		CChannelListButtons[3].button = NEUTRINO_ICON_BUTTON_DBOX;
	}
		
	frameBuffer->paintBoxRel(x, y + (height - buttonHeight), width, buttonHeight - 1, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - buttonHeight), ButtonWidth, sizeof(CChannelListButtons)/sizeof(CChannelListButtons[0]), CChannelListButtons, width);
}

void CChannelList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  chanlist[liststart]->number + listmaxshow;
	updateEvents();

	if(lastnum<10)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00000");

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}
	//scrollbar
	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((chanlist.size()- 1)/ listmaxshow)+ 1;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ sbs*(sb-4)/sbc, 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);
}

/*
CChannelList::CChannel* CChannelList::getChannelFromChannelID(const t_channel_id channel_id)
{
	for (std::vector<CChannel *>::iterator it = chanlist.begin(); it != chanlist.end(); it++)
	{
		if ((*it)->channel_id == channel_id)
			return (*it);
	}
	return NULL;
}
*/



// for EPG+  (2004-03-05 rasc, code sent by vivamiga)

int CChannelList::getSize() const
{
	return this->chanlist.size();
}

int CChannelList::getSelectedChannelIndex() const
{
	return this->selected;
}
