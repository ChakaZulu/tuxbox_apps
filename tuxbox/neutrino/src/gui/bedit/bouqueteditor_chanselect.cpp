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

#include <gui/bedit/bouqueteditor_chanselect.h>

#include <global.h>
#include <neutrino.h>

#include <gui/widget/buttons.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <gui/widget/icons.h>

#include <zapit/client/zapitclient.h>
#include <zapit/client/zapittools.h>


CBEChannelSelectWidget::CBEChannelSelectWidget(const std::string & Caption, unsigned int Bouquet, CZapitClient::channelsMode Mode)
	:CListBox(Caption.c_str())
{
	bouquet = Bouquet;
	mode =    Mode;
	// width =   500;
	// height =  440;
        width  = w_max (550, 0);
        height = h_max (440, 50);
	ButtonHeight = 25;
	theight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight     = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
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
	int ypos = y+ theight + paintNr*fheight;
	int c_rad_small;

	uint8_t    color;
	fb_pixel_t bgcolor;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		c_rad_small = RADIUS_SMALL;
	}
	else
	{
		color   = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
		c_rad_small = 0;
	}

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor, c_rad_small);

	if(itemNr < getItemCount())
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 8 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 8, ypos+ fheight, width - (8 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 8 + 8), Channels[itemNr].name, color);

		if( isChannelInBouquet(itemNr))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, x+8, ypos+4);
		}
		else
		{
			frameBuffer->paintBoxRel(x+8, ypos+4, NEUTRINO_ICON_BUTTON_GREEN_WIDTH, fheight-4, bgcolor);
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

int CBEChannelSelectWidget::exec(CMenuTarget* parent, const std::string & actionKey)
{
	Channels.clear();
	bouquetChannels.clear();
	g_Zapit->getBouquetChannels( bouquet, bouquetChannels, mode);
	g_Zapit->getChannels( Channels, mode, CZapitClient::SORT_ALPHA);

	return CListBox::exec(parent, actionKey);
}

void CBEChannelSelectWidget::paintHead()
{
	// paint head
	frameBuffer->paintBoxRel(x,y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight, width, ZapitTools::UTF8_to_Latin1(caption.c_str()) , COL_MENUHEAD);
}

const struct button_label CBEBouquetSelectWidgetButtons[2] =
{
	{ NEUTRINO_ICON_BUTTON_OKAY, LOCALE_BOUQUETEDITOR_SWITCH },
	{ NEUTRINO_ICON_BUTTON_HOME, LOCALE_BOUQUETEDITOR_RETURN }
};

void CBEChannelSelectWidget::paintFoot()
{
	int y_foot = y + height;
	frameBuffer->paintBoxRel(x, y_foot, width, ButtonHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);

	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 5, y_foot, width/2, 2, CBEBouquetSelectWidgetButtons);
}
