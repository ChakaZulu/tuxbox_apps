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

#include <gui/bedit/bouqueteditor_chanselect.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <gui/widget/icons.h>

#include <zapit/client/zapitclient.h>


CBEChannelSelectWidget::CBEChannelSelectWidget(std::string Caption, unsigned int Bouquet, CZapitClient::channelsMode Mode)
	:CListBox(Caption.c_str())
{
	bouquet = Bouquet;
	mode =    Mode;
	width =   500;
	height =  440;
}

uint	CBEChannelSelectWidget::getItemCount()
{
	return Channels.size();
}

bool CBEChannelSelectWidget::isChannelInBouquet( int index)
{
	for (unsigned int i=0; i<bouquetChannels.size(); i++)
	{
		if (bouquetChannels[i].channel_id == Channels[index].channel_id)
		{
			return true;
		}
	}
	return false;
}

bool CBEChannelSelectWidget::hasChanged()
{
	return modified;
}

void CBEChannelSelectWidget::paintItem(uint itemNr, int paintNr, bool selected)
{
	int ypos = y+ theight + paintNr*getItemHeight();
	int color = COL_MENUCONTENT;
	if (selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}
	frameBuffer->paintBoxRel(x,ypos, width- 15, getItemHeight(), color);

	if(itemNr < getItemCount())
	{
		g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, Channels[itemNr].name, color);

		if( isChannelInBouquet(itemNr))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, x+8, ypos+4);
		}
		else
		{
			frameBuffer->paintBoxRel(x+8,ypos+4, 16, fheight-4, color);
		}
	}
}


void CBEChannelSelectWidget::onOkKeyPressed()
{
	setModified();
	if (isChannelInBouquet(selected))
		g_Zapit->removeChannelFromBouquet( bouquet, Channels[selected].channel_id);
	else
		g_Zapit->addChannelToBouquet( bouquet, Channels[selected].channel_id);
	bouquetChannels.clear();
	g_Zapit->getBouquetChannels( bouquet, bouquetChannels, mode);
	
	paintItem( selected, selected - liststart, false);
	g_RCInput->postMsg( CRCInput::RC_down, 0 );
}

int CBEChannelSelectWidget::exec(CMenuTarget* parent, std::string actionKey)
{
	Channels.clear();
	bouquetChannels.clear();
	g_Zapit->getBouquetChannels( bouquet, bouquetChannels, mode);
	g_Zapit->getChannels( Channels, mode, CZapitClient::SORT_ALPHA);

	return CListBox::exec(parent, actionKey);
}


void CBEChannelSelectWidget::paintFoot()
{
	int ButtonWidth = width / 3;
	frameBuffer->paintBoxRel(x,y+height, width,ButtonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x+width- 3* ButtonWidth+ 8, y+height+1);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth+ 38, y+height+24 - 2, width, g_Locale->getText("bouqueteditor.switch"), COL_INFOBAR, 0, true); // UTF-8

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HOME, x+width - ButtonWidth+ 8, y+height+1);
	g_Fonts->infobar_small->RenderString(x+width - ButtonWidth+ 38, y+height+24 - 2, width, g_Locale->getText("bouqueteditor.return"), COL_INFOBAR, 0, true); // UTF-8
}
