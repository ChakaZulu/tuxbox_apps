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

#include <gui/bedit/bouqueteditor_bouquets.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <gui/bedit/bouqueteditor_channels.h>
#include <gui/widget/buttons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>

#include <zapit/client/zapitclient.h>
#include <zapit/client/zapittools.h>


CBEBouquetWidget::CBEBouquetWidget()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = 0;
	// width  = 500;
	// height = 440;
	// ButtonHeight = 25;
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
	state = beDefault;
	blueFunction = beRename;
}

void CBEBouquetWidget::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int c_rad_small;

	uint8_t    color;
	fb_pixel_t bgcolor;
	if (liststart + pos == selected)
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

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
		c_rad_small = RADIUS_SMALL;
	}

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor, c_rad_small);
	if ((liststart+pos==selected) && (state == beMoving))
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x + 8, ypos+4);
	}
	if(liststart+pos<Bouquets.size())
	{
		if (Bouquets[liststart+pos].locked)
		{
			frameBuffer->paintIcon("lock.raw", x + 7, ypos);
		}
		if (Bouquets[liststart+pos].hidden)
		{
			frameBuffer->paintIcon("hidden.raw", x + 37, ypos);
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+68, ypos+ fheight, width-68, Bouquets[liststart+pos].name, color);
	}
}

void CBEBouquetWidget::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((Bouquets.size()- 1)/ listmaxshow)+ 1;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ sbs*(sb-4)/sbc, 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);

}

void CBEBouquetWidget::paintHead()
{
	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight+0, width, g_Locale->getText(LOCALE_BOUQUETLIST_HEAD), COL_MENUHEAD, 0, true); // UTF-8
}

struct button_label CBEBouquetWidgetButtons1[5] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , 	LOCALE_BOUQUETEDITOR_DELETE	},
	{ NEUTRINO_ICON_BUTTON_GREEN , 	LOCALE_BOUQUETEDITOR_ADD	},
	{ NEUTRINO_ICON_BUTTON_YELLOW, 	LOCALE_BOUQUETEDITOR_MOVE	},
	{ NEUTRINO_ICON_BUTTON_BLUE,  	LOCALE_GENERIC_EMPTY 		}, // button without caption
	{ NEUTRINO_ICON_BUTTON_DBOX, 	LOCALE_BOUQUETEDITOR_RENAME	}
};

void CBEBouquetWidget::paintFoot()
{
	frameBuffer->paintBoxRel(x,y+height, width,ButtonHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	frameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW_PLUS_0);

	switch( blueFunction)
	{
		case beRename:
			CBEBouquetWidgetButtons1[4].locale = LOCALE_BOUQUETEDITOR_RENAME;
		break;
		case beHide:
			CBEBouquetWidgetButtons1[4].locale = LOCALE_BOUQUETEDITOR_HIDE;
		break;
		case beLock:
			CBEBouquetWidgetButtons1[4].locale = LOCALE_BOUQUETEDITOR_LOCK;
		break;
	}
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 5, y + height+2, (width-20)/5, 5, CBEBouquetWidgetButtons1, width-20);
}

void CBEBouquetWidget::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height+ButtonHeight);
}

int CBEBouquetWidget::exec(CMenuTarget* parent, const std::string&)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

// getting all bouquets from zapit
	Bouquets.clear();
	g_Zapit->getBouquets(Bouquets, true);
	paintHead();
	paint();
	paintFoot();

	bouquetsChanged = false;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

		if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
		{
			if (state == beDefault)
			{
				if (bouquetsChanged)
				{
					int result = ShowLocalizedMessage(LOCALE_BOUQUETEDITOR_NAME, LOCALE_BOUQUETEDITOR_SAVECHANGES, CMessageBox::mbrYes, CMessageBox::mbAll);

					switch( result )
					{
						case CMessageBox::mbrYes :
							loop=false;
							saveChanges();
						break;
						case CMessageBox::mbrNo :
							loop=false;
							discardChanges();
						break;
						case CMessageBox::mbrCancel :
							paintHead();
							paint();
							paintFoot();
						break;
					}
				}
				else
				{
					loop = false;
				}
			}
			else if (state == beMoving)
			{
				cancelMoveBouquet();
			}
		}
		//
		// -- For more convenience: include browsing of list (paging)  (rasc, 2002-04-02)
		// -- The keys should be configurable. Problem is: red/green key, which is the
		// -- default in neutrino is used as a function key here... so use left/right
		//
		else if (msg_repeatok==CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			if (!(Bouquets.empty()))
			{
				int step = 0;
				int prev_selected = selected;

				step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
				selected -= step;
				if((prev_selected-step) < 0)		// because of uint
				{
					selected = Bouquets.size()-1;
				}

				if (state == beDefault)
				{
					paintItem(prev_selected - liststart);
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
				else if (state == beMoving)
				{
					internalMoveBouquet(prev_selected, selected);
				}
			}
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			int step = 0;
			int prev_selected = selected;

			step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			selected += step;

			if(selected >= Bouquets.size())
			{
				selected = 0;
			}


			if (state == beDefault)
			{
				paintItem(prev_selected - liststart);
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
			else if (state == beMoving)
			{
				internalMoveBouquet(prev_selected, selected);
			}
		}
		else if(msg==CRCInput::RC_red)
		{
			if (state == beDefault)
				deleteBouquet();
		}
		else if(msg==CRCInput::RC_green)
		{
			if (state == beDefault)
				addBouquet();
		}
		else if(msg==CRCInput::RC_yellow)
		{
			if (selected < Bouquets.size()) /* Bouquets.size() might be 0 */
			{
				liststart = (selected/listmaxshow)*listmaxshow;
				if (state == beDefault)
					beginMoveBouquet();
				paintItem(selected - liststart);
			}
		}
		else if(msg==CRCInput::RC_blue)
		{
			if (selected < Bouquets.size()) /* Bouquets.size() might be 0 */
			{
				if (state == beDefault)
					switch (blueFunction)
					{
					case beRename:
						renameBouquet();
						break;
					case beHide:
						switchHideBouquet();
						break;
					case beLock:
						switchLockBouquet();
						break;
					}
			}
		}
		else if(msg==CRCInput::RC_setup)
		{
			if (state == beDefault)
			switch (blueFunction)
			{
				case beRename:
					blueFunction = beHide;
				break;
				case beHide:
					blueFunction = beLock;
				break;
				case beLock:
					blueFunction = beRename;
				break;
			}
			paintFoot();
		}
		else if(msg==CRCInput::RC_ok)
		{
			if (state == beDefault)
			{
				if (selected < Bouquets.size()) /* Bouquets.size() might be 0 */
				{
					CBEChannelWidget* channelWidget = new CBEChannelWidget(Bouquets[selected].name, selected);
					channelWidget->exec( this, "");
					if (channelWidget->hasChanged())
						bouquetsChanged = true;
					delete channelWidget;
					paintHead();
					paint();
					paintFoot();
				}
			}
			else if (state == beMoving)
			{
				finishMoveBouquet();
			}
		}
		else if( CRCInput::isNumeric(msg) )
		{
			if (state == beDefault)
			{
				//kein pushback - wenn man versehentlich wo draufkommt is die edit-arbeit umsonst
				//selected = oldselected;
				//g_RCInput->postMsg( msg, data );
				//loop=false;
			}
			else if (state == beMoving)
			{
				cancelMoveBouquet();
			}
		}
		else
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
			// kein canceling...
		}
	}
	hide();
	return res;
}

void CBEBouquetWidget::deleteBouquet()
{
	if (selected >= Bouquets.size()) /* Bouquets.size() might be 0 */
		return;

	g_Zapit->deleteBouquet(selected);
	Bouquets.clear();
	g_Zapit->getBouquets(Bouquets, true);
	if (selected >= Bouquets.size())
		selected = Bouquets.empty() ? 0 : (Bouquets.size() - 1);
	bouquetsChanged = true;
	paint();
}

void CBEBouquetWidget::addBouquet()
{
	std::string newName = inputName("", LOCALE_BOUQUETEDITOR_BOUQUETNAME);
	if (!(newName.empty()))
	{
		g_Zapit->addBouquet(ZapitTools::Latin1_to_UTF8(newName.c_str()).c_str());
		Bouquets.clear();
		g_Zapit->getBouquets(Bouquets, true);
		selected = Bouquets.empty() ? 0 : (Bouquets.size() - 1);
		bouquetsChanged = true;
	}
	paintHead();
	paint();
	paintFoot();
}

void CBEBouquetWidget::beginMoveBouquet()
{
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEBouquetWidget::finishMoveBouquet()
{
	state = beDefault;
	if (newPosition != origPosition)
	{
		g_Zapit->moveBouquet(origPosition, newPosition);
		Bouquets.clear();
		g_Zapit->getBouquets(Bouquets, true);
		bouquetsChanged = true;
	}
	paint();
}

void CBEBouquetWidget::cancelMoveBouquet()
{
	state = beDefault;
	internalMoveBouquet( newPosition, origPosition);
}

void CBEBouquetWidget::renameBouquet()
{
	std::string newName = inputName(Bouquets[selected].name, LOCALE_BOUQUETEDITOR_NEWBOUQUETNAME);
	if (newName != Bouquets[selected].name)
	{
		g_Zapit->renameBouquet(selected, ZapitTools::Latin1_to_UTF8(newName.c_str()).c_str());
		Bouquets.clear();
		g_Zapit->getBouquets(Bouquets, true);
		bouquetsChanged = true;
	}
	paintHead();
	paint();
	paintFoot();
}

void CBEBouquetWidget::switchHideBouquet()
{
	bouquetsChanged = true;
	Bouquets[selected].hidden = !Bouquets[selected].hidden;
	g_Zapit->setBouquetHidden(selected, Bouquets[selected].hidden);
	paint();
}

void CBEBouquetWidget::switchLockBouquet()
{
	bouquetsChanged = true;
	Bouquets[selected].locked = !Bouquets[selected].locked;
	g_Zapit->setBouquetLock(selected, Bouquets[selected].locked);
	paint();
}

std::string CBEBouquetWidget::inputName(const char * const defaultName, const neutrino_locale_t caption)
{
	char Name[30];

	strncpy(Name, defaultName, 30);

	CStringInputSMS * nameInput = new CStringInputSMS(caption, Name, 29, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/ ");
	nameInput->exec(this, "");
	delete nameInput;

	return std::string(Name);
}

void CBEBouquetWidget::internalMoveBouquet( unsigned int fromPosition, unsigned int toPosition)
{
	if ( (int) toPosition == -1 ) return;
	if ( toPosition == Bouquets.size()) return;

	CZapitClient::responseGetBouquets Bouquet = Bouquets[fromPosition];
	if (fromPosition < toPosition)
	{
		for (unsigned int i=fromPosition; i<toPosition; i++)
			Bouquets[i] = Bouquets[i+1];
	}
	else if (fromPosition > toPosition)
	{
		for (unsigned int i=fromPosition; i>toPosition; i--)
			Bouquets[i] = Bouquets[i-1];
	}
	Bouquets[toPosition] = Bouquet;
	selected = toPosition;
	newPosition = toPosition;
	paint();
}

void CBEBouquetWidget::saveChanges()
{
	CHintBox* hintBox= new CHintBox(LOCALE_BOUQUETEDITOR_NAME, g_Locale->getText(LOCALE_BOUQUETEDITOR_SAVINGCHANGES), 480); // UTF-8
	hintBox->paint();
	g_Zapit->saveBouquets();
	hintBox->hide();
	delete hintBox;
}

void CBEBouquetWidget::discardChanges()
{
	CHintBox* hintBox= new CHintBox(LOCALE_BOUQUETEDITOR_NAME, g_Locale->getText(LOCALE_BOUQUETEDITOR_DISCARDINGCHANGES), 480); // UTF-8
	hintBox->paint();
	g_Zapit->restoreBouquets();
	hintBox->hide();
	delete hintBox;
}
