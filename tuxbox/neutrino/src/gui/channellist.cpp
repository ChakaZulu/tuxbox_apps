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

#include "channellist.h"
#include "../global.h"

#define info_height 60

static char* copyStringto(const char* from, char* to, int len)
{
	const char *fromend=from+len;
	while(*from!='\n' && from<fromend && *from)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

CChannelList::CChannel::CChannel()
{
	bAlwaysLocked = false;
	bLockedProgramIsRunning = false;
}

// isCurrentlyLocked returns true if the channel is locked
// considering youth-protection-settings, bouquet-locking
// and currently running program
bool CChannelList::CChannel::isCurrentlyLocked()
{
	printf("bAlwaysLocked: %d, bLockedProgramIsRunning %d\n",bAlwaysLocked,bLockedProgramIsRunning );
	return ( bAlwaysLocked || bLockedProgramIsRunning);
//	return( true);
}

// lockedProgramStarts should be called when a locked program starts
void CChannelList::CChannel::lockedProgramStarts( uint age)
{
	if ((g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) && ( age >= g_settings.parentallock_lockage))
	{
		bLockedProgramIsRunning = true;
	}
}

// lockedProgramEnds should be called when a locked program ends
void CChannelList::CChannel::lockedProgramEnds()
{
	bLockedProgramIsRunning = false;
}


CChannelList::CChannelList( const std::string &Name )
{
	name = Name;
	selected = 0;
	width = 520;
	height = 420;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
	liststart = 0;
	tuned=0xfffffff;
}

CChannelList::~CChannelList()
{
	for(unsigned int count=0;count<chanlist.size();count++)
	{
		delete chanlist[count];
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
	else if ( nNewChannel = -1)
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

// quick'n dirty
void CChannelList::updateEvents(void)
{
	char rip[]="127.0.0.1";

	//printf("\n START CChannelList::updateEvents \n\n");
	int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SAI servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(sectionsd::portNumber);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to sectionsd!");
		return;
	}

	sectionsd::msgRequestHeader req;
	req.version = 2;

	req.command = sectionsd::actualEventListTVshortIDs;
	req.dataLength = 0;
	write(sock_fd,&req,sizeof(req));

	sectionsd::msgResponseHeader resp;
	memset(&resp, 0, sizeof(resp));
	if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
	{
		close(sock_fd);
		return;
	}
	if(resp.dataLength<=0)
	{
		close(sock_fd);
		return;
	}

	char* pData = new char[resp.dataLength] ;
	if ( recv(sock_fd, pData, resp.dataLength, MSG_WAITALL)!= resp.dataLength )
	{
		delete[] pData;
		close(sock_fd);
		return;
	}

	close(sock_fd);

	char *actPos = pData;

/*	FILE *file=fopen("channellist.list", "wb");
        if(file) {
            fwrite(pData,  resp.dataLength, 1, file);
            fclose(file);
        }
*/
	for(unsigned int count=0;count<chanlist.size();count++)
		chanlist[count]->currentEvent.description="";

	//printf("\n read finished CChannelList::updateEvents \n\n");

//	printf("data length: 0x%x\n", resp.dataLength);
	while(actPos<pData+resp.dataLength)
	{
		unsigned* serviceID = (unsigned*) actPos;
		actPos+=4;

		unsigned long long* evt_id = (unsigned long long*) actPos;
		actPos+=8;

		time_t* startt = (time_t*) actPos;
		actPos+=4;

		unsigned* dauert = (unsigned*) actPos;
		actPos+=4;

		string descriptiont= actPos;
		actPos+=strlen(actPos)+1;
		string textt= actPos;
		actPos+=strlen(actPos)+1;

		// quick'n dirty, sollte man mal anders machen
		for (unsigned int count=0;count<chanlist.size();count++)
		{
			if (chanlist[count]->onid_sid==*serviceID)
			{
				chanlist[count]->currentEvent.id= *evt_id;
				chanlist[count]->currentEvent.description= descriptiont;
				chanlist[count]->currentEvent.text_1= textt;
				chanlist[count]->currentEvent.startzeit= *startt;
				chanlist[count]->currentEvent.dauer= *dauert;
				//	printf("Channel found: %s\n", actPos);
				break;
			}
		}

	}

	delete[] pData;
	//printf("\n END CChannelList::updateEvents \n\n");
	return;
}

void CChannelList::addChannel(int key, int number, const std::string& name, unsigned int ids)
{
	CChannel* tmp = new CChannel();
	tmp->key=key;
	tmp->number=number;
	tmp->name=name;
	tmp->onid_sid=ids;
	tmp->bAlwaysLocked = false;
	tmp->bLockedProgramIsRunning = false;
	chanlist.insert(chanlist.end(), tmp);
}

void CChannelList::addChannel(CChannelList::CChannel* chan)
{
	if (chan!=NULL)
		chanlist.insert(chanlist.end(), chan);
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

void CChannelList::setName(const std::string& Name)
{
	name = Name;
}

int CChannelList::getKey(int id)
{
	return chanlist[id]->key;
}

const std::string& CChannelList::getActiveChannelName()
{
	return chanlist[selected]->name;
}

const std::string CChannelList::getActiveChannelID()
{
	string  id;
	char anid[10];
	snprintf( anid, 10, "%x", getActiveChannelOnid_sid() );
	id= anid;
	return id;
}

int CChannelList::getActiveChannelNumber()
{
	return selected+1;
}

int CChannelList::show()
{
	int res = -1;

	if(chanlist.size()==0)
	{
		//evtl. anzeige dass keine kanalliste....
		return res;
	}
	paintHead();
	updateEvents();
	paint();

	int oldselected = selected;
	int zapOnExit = false;
	bool bShowBouquetList = false;
	bool loop=true;
	while (loop)
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, g_settings.timing_chanlist );

		if ( ( msg == CRCInput::RC_timeout ) ||
			 ( msg == g_settings.key_channelList_cancel) )
		{
			selected = oldselected;
			loop=false;
		}
		else if ( msg == g_settings.key_channelList_pageup )
		{
			selected+=listmaxshow;
			if (selected>chanlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == g_settings.key_channelList_pagedown )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=chanlist.size()-1;
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
				selected = chanlist.size()-1;
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
			selected = (selected+1)%chanlist.size();
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
		else if ( ( msg == g_settings.key_bouquet_up ) && ( bouquetList != NULL ) )
		{
			if (bouquetList->Bouquets.size() > 0)
			{
				int nNext = (bouquetList->getActiveBouquetNumber()+1) % bouquetList->Bouquets.size();
				bouquetList->activateBouquet( nNext );
				res = bouquetList->showChannelList();
				loop = false;
			}
		}
		else if ( ( msg == g_settings.key_bouquet_down ) && ( bouquetList != NULL ) )
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
		else if( (msg==CRCInput::RC_red) ||
				 (msg==CRCInput::RC_green) ||
				 (msg==CRCInput::RC_yellow) ||
				 (msg==CRCInput::RC_blue) ||
		         (CRCInput::isNumeric(msg)) )
		{
			//pushback key if...
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else if ( msg == CRCInput::RC_help )
		{
			hide();

			if ( g_EventList->exec(chanlist[selected]->onid_sid, chanlist[selected]->name ) == menu_return::RETURN_EXIT_ALL )
			{
				res = -2;
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
		else
		{
			if ( neutrino->handleMsg( msg, data ) == messages_return::cancel_all )
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
	g_FrameBuffer->paintBackgroundBoxRel(x, y, width, height+ info_height+ 5);
        clearItem2DetailsLine ();
}

bool CChannelList::showInfo(int pos)
{
	if((pos >= (signed int) chanlist.size()) || (pos<0))
	{
		return false;
	}

	CChannel* chan = chanlist[pos];
	g_InfoViewer->showTitle(pos+1, chan->name, chan->onid_sid, true );
	return true;
}

bool CChannelList::handleLockage( CChannel* chan)
{
	printf("[neutrino] handleLockage\n");
	if (chan->isCurrentlyLocked())
	{
		printf("[neutrino] channel is locked\n");
//		g_Zapit->stopPlayBack();
		CZapProtection zapProtection( g_settings.parentallock_pincode);
		if (!zapProtection.check())
		{
			if (bouquetList != NULL)
				bouquetList->adjustToChannel( getActiveChannelNumber());
			g_InfoViewer->killTitle(); // in case we came from numzap
			return( false);
		}
	}
//	g_Zapit->startPlayBack();
	return( true);
}


//
// -- Zap to channel with onid_sid
// -- 2002-04-14 rasc
//
//
void CChannelList::zapToOnidSid (unsigned int onid_sid)
{
	int i;

	for (i=0; i<chanlist.size(); i++) {
		if (chanlist[i]->onid_sid == onid_sid) {
			zapTo (i);
			break;
		}
	}

}



void CChannelList::zapTo(int pos)
{
	if (chanlist.size() == 0)
	{
		ShowMsg ( "messagebox.error", g_Locale->getText("channellist.nonefound"), CMessageBox::mbrCancel, CMessageBox::mbCancel );
		return;
	}
	if ( (pos >= (signed int) chanlist.size()) || (pos< 0) )
	{
		pos = 0;
	}

	if (!handleLockage( chanlist[pos]))
	{
		return;
	}

	selected= pos;
	CChannel* chan = chanlist[selected];
	lastChList.store (selected);

	if ( pos!=(int)tuned )
	{
		tuned = pos;
		g_RemoteControl->zapTo_onid_sid( chan->onid_sid, chan->name );
	}
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	if (bouquetList != NULL)
		bouquetList->adjustToChannel( getActiveChannelNumber());
}

int CChannelList::numericZap(int key)
{
	int res = menu_return::RETURN_REPAINT;

	if(chanlist.size()==0)
	{
		ShowMsg ( "messagebox.error", g_Locale->getText("channellist.nonefound"), CMessageBox::mbrCancel, CMessageBox::mbCancel );
		return res;
	}

	//schneller zap mit "0" taste zwischen den letzten beiden sendern...
	if( key == 0 )
	{
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

	int ox=300;
	int oy=200;
	int sx = g_Fonts->channel_num_zap->getRenderWidth("0000")+14;
	int sy = g_Fonts->channel_num_zap->getHeight()+6;
	char valstr[10];
	int chn=key;
	int lastchan= -1;
	int pos=1;
	uint msg; uint data;
	bool doZap = true;


	while(1)
	{
		if (lastchan != chn)
		{
			sprintf((char*) &valstr, "%d", chn);
			while(strlen(valstr)<4)
				strcat(valstr,"·");   //"_"

			g_FrameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR);

			for (int i=3; i>=0; i--)
			{
				valstr[i+ 1]= 0;
				g_Fonts->channel_num_zap->RenderString(ox+7+ i*((sx-14)>>2), oy+sy-3, sx, &valstr[i], COL_INFOBAR);
			}

			showInfo(chn- 1);
		}


		g_RCInput->getMsg( &msg, &data, 30 );

		if ( msg == CRCInput::RC_timeout )
		{
			if ( ( chn > (int)chanlist.size() ) || (chn == 0) )
				chn = tuned + 1;
			break;
		}
		else if ( ( msg >= 0 ) && ( msg <= 9 ) )
		{ //numeric
			if ( pos==4 )
			{
				chn = msg;
				pos = 0;
			}
			else
				chn = chn* 10 + msg;

			pos++;
		}
		else if ( msg == CRCInput::RC_ok )
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
			{
				chn = tuned + 1;
			}
			break;
		}
		else if ( msg == g_settings.key_quickzap_down )
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
		else if ( msg == g_settings.key_quickzap_up )
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
		else if ( neutrino->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			doZap = false;
			res = menu_return::RETURN_EXIT_ALL;
			break;
		}
	}

	g_FrameBuffer->paintBackgroundBoxRel(ox, oy, sx, sy);

	if ( doZap )
	{
		chn--;
		if (chn<0)
			chn=0;
		zapTo( chn );
	}
	else
	{
		g_InfoViewer->killTitle();
	}

	return res;
}

void CChannelList::quickZap(int key)
{
        if(chanlist.size()==0)
        {
                //evtl. anzeige dass keine kanalliste....
                return;
        }

        if (key==g_settings.key_quickzap_down)
        {
                if(selected==0)
                        selected = chanlist.size()-1;
                else
                        selected--;
                //                              CChannel* chan = chanlist[selected];
        }
        else if (key==g_settings.key_quickzap_up)
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

void CChannelList::paintDetails(int index)
{
	if ( chanlist[index]->currentEvent.description== "" )
	{
		g_FrameBuffer->paintBackgroundBoxRel(x, y+ height, width, info_height);
	}
	else
	{
		// löschen
		g_FrameBuffer->paintBoxRel(x, y+ height, width, info_height, COL_MENUCONTENTDARK);

		char cNoch[50];
		char cSeit[50];

        struct		tm *pStartZeit = localtime(&chanlist[index]->currentEvent.startzeit);
        unsigned 	seit = ( time(NULL) - chanlist[index]->currentEvent.startzeit ) / 60;
        sprintf( cSeit, g_Locale->getText("channellist.since").c_str(), pStartZeit->tm_hour, pStartZeit->tm_min); //, seit );
        int seit_len= g_Fonts->channellist_descr->getRenderWidth(cSeit);

        int noch = ( chanlist[index]->currentEvent.startzeit + chanlist[index]->currentEvent.dauer - time(NULL)   ) / 60;
        if ( (noch< 0) || (noch>=10000) )
        	noch= 0;
        sprintf( cNoch, "(%d / %d min)", seit, noch );
        int noch_len= g_Fonts->channellist_number->getRenderWidth(cNoch);

		string text1= chanlist[index]->currentEvent.description;
		string text2= chanlist[index]->currentEvent.text_1;

		int xstart = 10;
		if ( g_Fonts->channellist->getRenderWidth(text1.c_str())> (width - 30 - seit_len) )
		{
			// zu breit, Umbruch versuchen...
		    int pos;
		    do
		    {
				pos = text1.find_last_of("[ -.]+");
				if ( pos!=-1 )
					text1 = text1.substr( 0, pos );
			} while ( ( pos != -1 ) && ( g_Fonts->channellist->getRenderWidth(text1.c_str())> (width - 30 - seit_len) ) );

			string text3= chanlist[index]->currentEvent.description.substr(text1.length()+ 1).c_str();
			if ( text2 != "" )
				text3= text3+ " · ";

			xstart+= g_Fonts->channellist->getRenderWidth(text3.c_str());
			g_Fonts->channellist->RenderString(x+ 10, y+ height+ 5+ 2* fheight, width - 30- noch_len, text3.c_str(), COL_MENUCONTENTDARK);
		}

		if ( text2 != "" )
		{
			while ( text2.find_first_of("[ -.+*#?=!$%&/]+") == 0 )
				text2 = text2.substr( 1 );
			text2 = text2.substr( 0, text2.find("\n") );
			g_Fonts->channellist_descr->RenderString(x+ xstart, y+ height+ 5+ 2* fheight, width- xstart- 20- noch_len, text2.c_str(), COL_MENUCONTENTDARK);
		}

		g_Fonts->channellist->RenderString(x+ 10, y+ height+ 5+ fheight, width - 30 - seit_len, text1.c_str(), COL_MENUCONTENTDARK);
		g_Fonts->channellist_descr->RenderString(x+ width- 10- seit_len, y+ height+ 5+ fheight, seit_len, cSeit, COL_MENUCONTENTDARK);

		g_Fonts->channellist_number->RenderString(x+ width- 10- noch_len, y+ height+ 5+ 2* fheight- 2, noch_len, cNoch, COL_MENUCONTENTDARK);
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

void CChannelList::paintItem2DetailsLine (int pos, int ch_index)
{
	#define ConnectLineBox_Width	16

	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + theight+0 + pos*fheight;
	int ypos2 = y + height;
	int ypos1a = ypos1 + (fheight/2)-2;
	int ypos2a = ypos2 + (info_height/2)-2;
	unsigned char col1 = COL_MENUCONTENT+6;
	unsigned char col2 = COL_MENUCONTENT+1;


	// Clear
	g_FrameBuffer->paintBackgroundBoxRel(xpos,y, ConnectLineBox_Width, height+info_height);

	// paint Line if detail info (and not valid list pos)
	if (pos >= 0 &&  chanlist[ch_index]->currentEvent.description != "")
	{
		// 1. col thick line
		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 4,fheight,     col1);
		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 4,info_height, col1);

		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 4,ypos2a-ypos1a, col1);

		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,4, col1);
		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos2a, 12,4, col1);

		// 2. col small line
		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 1,fheight,     col2);
		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 1,info_height, col2);

		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 1,ypos2a-ypos1a+4, col2);

		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,1, col2);
		g_FrameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-12, ypos2a, 8,1, col2);

		// -- small Frame around infobox
                g_FrameBuffer->paintBoxRel(x,         ypos2, 2,info_height, col1);
                g_FrameBuffer->paintBoxRel(x+width-2, ypos2, 2,info_height, col1);
                g_FrameBuffer->paintBoxRel(x        , ypos2, width-2,2,     col1);
                g_FrameBuffer->paintBoxRel(x        , ypos2+info_height-2, width-2,2, col1);

	}

}




void CChannelList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
		paintDetails(liststart+pos);
		paintItem2DetailsLine (pos, liststart+pos);
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);
	if(liststart+pos<chanlist.size())
	{
		CChannel* chan = chanlist[liststart+pos];
		//number
		char tmp[10];
		sprintf((char*) tmp, "%d", chan->number);

		int numpos = x+5+numwidth- g_Fonts->channellist_number->getRenderWidth(tmp);
		g_Fonts->channellist_number->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);
		if(strlen(chan->currentEvent.description.c_str()))
		{
			char nameAndDescription[100];
			snprintf(nameAndDescription, sizeof(nameAndDescription), "%s · ", chan->name.c_str());

			int ch_name_len= g_Fonts->channellist->getRenderWidth(nameAndDescription);
			int ch_desc_len= g_Fonts->channellist_descr->getRenderWidth(chan->currentEvent.description.c_str());

			if ( (width- numwidth- 20- 15- ch_name_len)< ch_desc_len )
				ch_desc_len = (width- numwidth- 20- 15- ch_name_len);
			if (ch_desc_len< 0)
				ch_desc_len = 0;

			g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, nameAndDescription, color);


			// rechtsbündig - auskommentiert
			// g_Fonts->channellist_descr->RenderString(x+ width- 15- ch_desc_len, ypos+ fheight, ch_desc_len, chan->currentEvent.description.c_str(), color);

			// linksbündig
			g_Fonts->channellist_descr->RenderString(x+ 5+ numwidth+ 10+ ch_name_len+ 5, ypos+ fheight, ch_desc_len, chan->currentEvent.description.c_str(), color);
		}
		else
			//name
			g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, chan->name.c_str(), color);
	}
}

void CChannelList::paintHead()
{
	string strCaption = g_Locale->getText(name).c_str();

	if (strCaption == "")
	{
		strCaption = name;
	}
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width- 65, strCaption.c_str(), COL_MENUHEAD);

	g_FrameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );
	if (bouquetList!=NULL)
		g_FrameBuffer->paintIcon("dbox.raw", x+ width- 60, y+ 5 );
}

void CChannelList::paint()
{
	g_Sectionsd->setPauseSorting( true );

	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  chanlist[liststart]->number + listmaxshow;

	if(lastnum<10)
		numwidth = g_Fonts->channellist_number->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Fonts->channellist_number->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("00000");

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	g_FrameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((chanlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

	g_Sectionsd->setPauseSorting( false );
}

CChannelList::CChannel* CChannelList::getChannelFromOnidSid(int onidSid)
{
	for (uint i=0; i< chanlist.size();i++)
	{
		if (chanlist[i]->onid_sid == onidSid)
			return chanlist[i];
	}
	return(NULL);
}

