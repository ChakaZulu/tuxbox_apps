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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/bouquetlist.h>

#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/infoviewer.h>

#include <gui/widget/menue.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>

#include <global.h>
#include <neutrino.h>


CBouquetList::CBouquetList()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected    = 0;
	liststart   = 0;
}

CBouquetList::~CBouquetList()
{
	Bouquets.clear();
}

CBouquet* CBouquetList::addBouquet(const char * const name, int BouquetKey, bool locked)
{
	if ( BouquetKey==-1 )
		BouquetKey= Bouquets.size();

	CBouquet* tmp = new CBouquet( BouquetKey, name, locked );
	Bouquets.push_back(tmp);
	return(tmp);
}

int CBouquetList::getActiveBouquetNumber()
{
	return selected;
}

int CBouquetList::showChannelList( int nBouquet)
{
	if (nBouquet == -1)
		nBouquet = selected;

	int nNewChannel = Bouquets[nBouquet]->channelList->show();
	if (nNewChannel > -1)
	{
		selected = nBouquet;
		orgChannelList->zapTo(Bouquets[selected]->channelList->getKey(nNewChannel)-1);

		nNewChannel= -2; // exit!
	}

	return nNewChannel;
}

void CBouquetList::adjustToChannel( int nChannelNr)
{
	for (uint i=0; i<Bouquets.size(); i++)
	{
		int nChannelPos = CNeutrinoApp::getInstance ()->recordingstatus ?nChannelNr-1: Bouquets[i]->channelList->hasChannel(nChannelNr);
		if (nChannelPos > -1)
		{
			selected = i;
			Bouquets[i]->channelList->setSelected(nChannelPos);
			return;
		}
	}
}


int CBouquetList::activateBouquet( int id, bool bShowChannelList)
{
	int res = menu_return::RETURN_REPAINT;

	selected = id;
	if (bShowChannelList)
	{
		int nNewChannel = Bouquets[selected]->channelList->show();

		if (nNewChannel > -1)
		{
			orgChannelList->zapTo(Bouquets[selected]->channelList->getKey(nNewChannel)-1);
		}
		else if ( nNewChannel == -2 )
		{
			// -2 bedeutet EXIT_ALL
			res = menu_return::RETURN_EXIT_ALL;
		}
	}

	return res;
}

int CBouquetList::exec( bool bShowChannelList)
{
    int res= show();

	if ( res > -1)
	{
		return activateBouquet( selected, bShowChannelList );
	}
	else if ( res == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}

	return res;
}

int CBouquetList::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	width  = w_max (500, 0);
	height = h_max (440, 40);

	theight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight     = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height      = theight + listmaxshow * fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	if(Bouquets.size()==0)
	{
		return res;
	}

	int maxpos= 1;
	int i= Bouquets.size();
	while ((i= i/10)!=0)
		maxpos++;

	paintHead();
	paint();

	int oldselected = selected;
	int firstselected = selected+ 1;
	int zapOnExit = false;

	unsigned int chn= 0;
	int pos= maxpos;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if ((msg == CRCInput::RC_timeout                             ) ||
		    (msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
		{
			selected = oldselected;
			loop=false;
		}
		else if ((msg==CRCInput::RC_up || msg==(neutrino_msg_t)g_settings.key_channelList_pageup))
		{
			int step = 0;
			int prev_selected = selected;

			step = (msg == (neutrino_msg_t)g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
			selected -= step;
			if((prev_selected-step) < 0)		// because of uint
				selected = Bouquets.size()-1;

			paintItem(prev_selected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
				paint();
			else
				paintItem(selected - liststart);
		}
		else if ((msg==CRCInput::RC_down || msg==(neutrino_msg_t)g_settings.key_channelList_pagedown))
		{
			int step = 0;
			int prev_selected = selected;

			step = (msg == (neutrino_msg_t)g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			selected += step;

			if(selected >= Bouquets.size())
				selected = 0;

			paintItem(prev_selected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
				paint();
			else
				paintItem(selected - liststart);
		}
		else if ( msg == CRCInput::RC_ok )
		{
			zapOnExit = true;
			loop=false;
		}
		else if (CRCInput::isNumeric(msg))
		{
			if (pos == maxpos)
			{
				if (msg == CRCInput::RC_0)
				{
					chn = firstselected;
					pos = maxpos;
				}
				else
				{
					chn = CRCInput::getNumericValue(msg);
					pos = 1;
				}
			}
			else
			{
				chn = chn * 10 + CRCInput::getNumericValue(msg);
				pos++;
			}

			if (chn > Bouquets.size())
			{
				chn = firstselected;
				pos = maxpos;
			}

			int prevselected=selected;
			selected = (chn - 1) % Bouquets.size(); // is % necessary (i.e. can firstselected be > Bouquets.size()) ?
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
		else if( ( msg == CRCInput::RC_red ) ||
				 ( msg == CRCInput::RC_green ) ||
				 ( msg == CRCInput::RC_yellow ) ||
				 ( msg == CRCInput::RC_blue ) )
		{
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = -2;
			}
		};
	}
	hide();
	if(zapOnExit)
	{
		return (selected);
	}
	else
	{
		return (res);
	}
}

void CBouquetList::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


void CBouquetList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;

	uint8_t    color;
	fb_pixel_t bgcolor;
	if (liststart + pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else
	{
		color   = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
	}

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor);
	if(liststart+pos<Bouquets.size())
	{
		CBouquet* bouq = Bouquets[liststart+pos];
		//number - zum direkten hinhüpfen
		char tmp[10];
		sprintf((char*) tmp, "%d", liststart+pos+ 1);

		int numpos = x+5+numwidth- g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);

		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, bouq->channelList->getName(), color, 0, true); // UTF-8
	}
}

void CBouquetList::paintHead()
{
	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight+0, width, g_Locale->getText(LOCALE_BOUQUETLIST_HEAD), COL_MENUHEAD, 0, true); // UTF-8
}

void CBouquetList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;

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

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((Bouquets.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3);
}
