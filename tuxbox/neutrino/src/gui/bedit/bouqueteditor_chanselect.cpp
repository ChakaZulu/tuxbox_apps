/*
$Id: bouqueteditor_chanselect.cpp,v 1.12 2002/04/05 01:14:43 rasc Exp $


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


$Log: bouqueteditor_chanselect.cpp,v $
Revision 1.12  2002/04/05 01:14:43  rasc
-- Favorites Bouquet handling (Easy Add Channels)



*/

#include "bouqueteditor_chanselect.h"
#include "../global.h"

CBEChannelSelectWidget::CBEChannelSelectWidget(string Caption, unsigned int Bouquet, CZapitClient::channelsMode Mode)
{
	selected = 0;
	width = 500;
	height = 440;
	ButtonHeight = 25;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
	caption = Caption;
	bouquet = Bouquet;
	mode = Mode;
}

void CBEChannelSelectWidget::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);
	if(liststart+pos < Channels.size())
	{
		g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, Channels[liststart+pos].name, color);
	}
	if( isChannelInBouquet( liststart+pos))
	{
		g_FrameBuffer->paintIcon("gruen.raw", x+8, ypos+4);
	}
	else
	{
		g_FrameBuffer->paintBackgroundBoxRel(x+8,ypos+4, 16, fheight-4);
		g_FrameBuffer->paintBoxRel(x+8,ypos+4, 16, fheight-4, color);
	}
}

void CBEChannelSelectWidget::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;

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

	int sbc= ((Channels.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}

void CBEChannelSelectWidget::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, caption.c_str() , COL_MENUHEAD);
}

void CBEChannelSelectWidget::paintFoot()
{
	int ButtonWidth = width / 3;
	g_FrameBuffer->paintBoxRel(x,y+height, width,ButtonHeight, COL_MENUHEAD);
	g_FrameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW);

	g_FrameBuffer->paintIcon("ok.raw", x+width- 3* ButtonWidth+ 8, y+height+1);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth+ 38, y+height+24 - 2, width, g_Locale->getText("bouqueteditor.switch").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("home.raw", x+width - ButtonWidth+ 8, y+height+1);
	g_Fonts->infobar_small->RenderString(x+width - ButtonWidth+ 38, y+height+24 - 2, width, g_Locale->getText("bouqueteditor.return").c_str(), COL_INFOBAR);

}

void CBEChannelSelectWidget::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height+ButtonHeight);
}

int CBEChannelSelectWidget::exec(CMenuTarget* parent, string actionKey)
{
	int res = menu_return::RETURN_REPAINT;
	if (parent)
	{
		parent->hide();
	}

	printf("clearing\n");
	Channels.clear();
	bouquetChannels.clear();
	printf("getting bc\n");
	g_Zapit->getBouquetChannels( bouquet, bouquetChannels, mode);
	printf("getting c\n");
	g_Zapit->getChannels( Channels, mode, CZapitClient::SORT_ALPHA);
	printf("getting done\n");
	paintHead();
	paint();
	paintFoot();

	channelsChanged = false;
	int oldselected = selected;
	bool loop=true;
	while (loop)
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, g_settings.timing_epg );

		if (( msg ==g_settings.key_channelList_cancel) || ( msg ==CRCInput::RC_home))
		{
			loop = false;
		}
		else if ( msg ==CRCInput::RC_up)
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = Channels.size()-1;
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
		else if ( msg ==CRCInput::RC_down)
		{
			rcDown();
		}
		else if ( msg ==g_settings.key_channelList_pageup)
		{
			selected+=listmaxshow;
			if (selected>Channels.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg ==g_settings.key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=Channels.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if( msg ==CRCInput::RC_ok)
		{
			switchChannel();
		}
		//kein pushback - wenn man versehentlich wo draufkommt is die edit-arbeit umsonst
		/*
		else if( CRCInput::isNumeric(key) )
		{
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop=false;
		}*/
		else
		{
			neutrino->handleMsg( msg, data );
			// kein canceling...
		}
	}
	hide();
	return res;
}

void CBEChannelSelectWidget::switchChannel()
{
	channelsChanged = true;
	if (isChannelInBouquet(selected))
	{
		g_Zapit->removeChannelFromBouquet( bouquet, Channels[selected].onid_sid);
	}
	else
	{
		g_Zapit->addChannelToBouquet( bouquet, Channels[selected].onid_sid);
	}
	bouquetChannels.clear();
	g_Zapit->getBouquetChannels( bouquet, bouquetChannels, mode);
	rcDown();
//	paintItem( selected - liststart);
}

bool CBEChannelSelectWidget::isChannelInBouquet( int index)
{
	for (unsigned int i=0; i<bouquetChannels.size(); i++)
	{
		if (bouquetChannels[i].onid_sid == Channels[index].onid_sid)
		{
			return true;
		}
	}
	return false;
}

bool CBEChannelSelectWidget::hasChanged()
{
	return (channelsChanged);
}

void CBEChannelSelectWidget::rcDown()
{
	int prevselected=selected;
	selected = (selected+1)%Channels.size();
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

