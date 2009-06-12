/*
	$Id: menue.cpp,v 1.153 2009/06/12 19:27:29 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	(C) 2008, 2009 Stefan Seyfried

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

#include <gui/widget/menue.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>

#include <gui/widget/stringinput.h>

#include <global.h>
#include <neutrino.h>

#include <cctype>

/* the following generic menu items are integrated into multiple menus at the same time */
CMenuSeparator CGenericMenuSeparator;
CMenuSeparator CGenericMenuSeparatorLine(CMenuSeparator::LINE);
CMenuForwarder CGenericMenuBack(LOCALE_MENU_BACK);
CMenuSeparator * const GenericMenuSeparator = &CGenericMenuSeparator;
CMenuSeparator * const GenericMenuSeparatorLine = &CGenericMenuSeparatorLine;
CMenuForwarder * const GenericMenuBack = &CGenericMenuBack;



void CMenuItem::init(const int X, const int Y, const int DX, const int OFFX)
{
	x    = X;
	y    = Y;
	dx   = DX;
	offx = OFFX;
}

void CMenuItem::setActive(const bool Active)
{
	active = Active;
	if (x != -1)
		paint();
}

CMenuWidget::CMenuWidget()
{
	nameString = g_Locale->getText(NONEXISTANT_LOCALE);
	iconfile="";
	selected=-1;
	iconOffset= 0;
}


CMenuWidget::CMenuWidget(const neutrino_locale_t Name, const std::string & Icon, const int mwidth, const int mheight)
{
	frameBuffer = CFrameBuffer::getInstance();
	nameString = g_Locale->getText(Name);
	iconfile = Icon;
	selected = -1;
	width = mwidth;
	if(width > (g_settings.screen_EndX - g_settings.screen_StartX))
		width = g_settings.screen_EndX - g_settings.screen_StartX;
	height = mheight; // height(menu_title)+10+...
	wanted_height=mheight;
	current_page=0;
}

CMenuWidget::CMenuWidget(const char* Name, const std::string & Icon, const int mwidth, const int mheight)
{
	frameBuffer = CFrameBuffer::getInstance();
	nameString = Name;
	iconfile = Icon;
	selected = -1;
	width = mwidth;
	if(width > (g_settings.screen_EndX - g_settings.screen_StartX))
		width = g_settings.screen_EndX - g_settings.screen_StartX;
	height = mheight; // height(menu_title)+10+...
	wanted_height=mheight;
	current_page=0;
}

CMenuWidget::~CMenuWidget()
{
	for(unsigned int count=0;count<items.size();count++)
	{
		CMenuItem * item = items[count];
		if ((item != GenericMenuSeparator) &&
		    (item != GenericMenuSeparatorLine) &&
		    (item != GenericMenuBack))
			delete item;
	}
	items.clear();
	page_start.clear();
}

void CMenuWidget::addItem(CMenuItem* menuItem, const bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	items.push_back(menuItem);
}

bool CMenuWidget::hasItem()
{
	return !items.empty();
}

int CMenuWidget::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	// paint() changes the mode...
	CLCD::MODES oldLcdMode = CLCD::getInstance()->getMode();
	std::string oldLcdMenutitle = CLCD::getInstance()->getMenutitle();

	int pos;

	if (parent)
		parent->hide();

	paint();
	int retval = menu_return::RETURN_REPAINT;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	do
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);


		if ( msg <= CRCInput::RC_MaxRC )
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
		}

		int handled= false;

		for (unsigned int i= 0; i< items.size(); i++)
		{
			CMenuItem* titem = items[i];
			if ((titem->directKey != CRCInput::RC_nokey) &&
			    (titem->directKey == msg))
			{
				if (titem->isSelectable())
				{
					items[selected]->paint( false );
					selected= i;
					msg= CRCInput::RC_ok;
				}
				else
				{
					// swallow-key...
					handled= true;
				}
				break;
			}
		}

		if (!handled)
		{
			switch (msg)
			{

				case (CRCInput::RC_up) :
				case (CRCInput::RC_up|CRCInput::RC_Repeat) :
				case (CRCInput::RC_down) :
				case (CRCInput::RC_down|CRCInput::RC_Repeat) :
					{
						//search next / prev selectable item
						for (unsigned int count=1; count< items.size(); count++)
						{

							if ((msg & ~CRCInput::RC_Repeat) == CRCInput::RC_up)
							{
								pos = selected- count;
								if ( pos<0 )
									pos += items.size();
							}
							else
							{
								pos = (selected+ count)%items.size();
							}

							CMenuItem* item = items[pos];

							if ( item->isSelectable() )
							{
								if ((pos < (int)page_start[current_page + 1]) &&
								    (pos >= (int)page_start[current_page]))
								{ // Item is currently on screen
									//clear prev. selected
									items[selected]->paint( false );
									//select new
									item->paint( true );
									selected = pos;
									break;
								}
								else
								{
									selected=pos;
									paintItems();
									break;
								}
							}
						}
					}
					break;
				case (CRCInput::RC_ok):
					{
						//exec this item...
						if ( hasItem() )
						{
						CMenuItem* item = items[selected];
						int rv = item->exec( this );
						switch ( rv )
						{
							case menu_return::RETURN_EXIT_ALL:
								retval = menu_return::RETURN_EXIT_ALL;

							case menu_return::RETURN_EXIT:
								msg = CRCInput::RC_timeout;
								break;
							case menu_return::RETURN_REPAINT:
								paint();
								break;
						}
					}
						else
						{
							msg = CRCInput::RC_timeout;
							break;
						}
					}
					break;

				case (CRCInput::RC_home):
					msg = CRCInput::RC_timeout;
					break;

				case (CRCInput::RC_right):
					break;

				case (CRCInput::RC_left):
					msg = CRCInput::RC_timeout;
					break;

				case (CRCInput::RC_timeout):
					break;

				//close any menue on dbox-key
				case (CRCInput::RC_setup):
					{
						msg = CRCInput::RC_timeout;
						retval = menu_return::RETURN_EXIT_ALL;
					}
					break;

				default:
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
					{
						retval = menu_return::RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
			}


			if ( msg <= CRCInput::RC_MaxRC )
			{
				// recalculate timeout for RC-keys
				timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
			}
		}

	}
	while ( msg!=CRCInput::RC_timeout );

	hide();
	if (CLCD::getInstance()->getMode() != CLCD::MODE_STANDBY)
		CLCD::getInstance()->setMode(oldLcdMode,oldLcdMenutitle.c_str());

	return retval;
}

void CMenuWidget::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width + 15, height + CORNER_RADIUS_MID * 2 + 1);
}

void CMenuWidget::paint()
{
	const char * l_name = nameString.c_str();

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, l_name);

	height=wanted_height;
	if(height > (g_settings.screen_EndY - g_settings.screen_StartY))
		height = g_settings.screen_EndY - g_settings.screen_StartY;

	int neededWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(l_name, true); // UTF-8
	if (neededWidth> width-48)
	{
		width= neededWidth+ 49;
		if(width > (g_settings.screen_EndX - g_settings.screen_StartX))
			width = g_settings.screen_EndX - g_settings.screen_StartX;
	}
	int hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	int itemHeightTotal=0;
	int heightCurrPage=0;
	page_start.clear();
	page_start.push_back(0);
	total_pages=1;
	for (unsigned int i= 0; i< items.size(); i++)
	{
		int item_height=items[i]->getHeight();
		itemHeightTotal+=item_height;
		heightCurrPage+=item_height;
		if(heightCurrPage > (height-hheight))
		{
			page_start.push_back(i);
			total_pages++;
			heightCurrPage=item_height;
		}
	}
	page_start.push_back(items.size());

	iconOffset= 0;
	for (unsigned int i= 0; i< items.size(); i++)
	{
		if ((!(items[i]->iconName.empty())) || CRCInput::isNumeric(items[i]->directKey))
		{
			iconOffset = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
			break;
		}
	}

	// shrink menu if less items
	if(hheight+itemHeightTotal < height)
		height=hheight+itemHeightTotal;

	y= ( ( ( g_settings.screen_EndY- g_settings.screen_StartY ) - height) >> 1 ) + g_settings.screen_StartY;
	x= ( ( ( g_settings.screen_EndX- g_settings.screen_StartX ) - width ) >> 1 ) + g_settings.screen_StartX;

	int sb_width;
	if(total_pages > 1)
		sb_width=15;
	else
		sb_width=0;

	int c_rad_mid = RADIUS_MID;
	
	frameBuffer->paintBoxRel(x, y + height - ((c_rad_mid * 2) + 1) + (c_rad_mid / 3 * 2), width + sb_width, ((c_rad_mid * 2) + 1), COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);
	frameBuffer->paintBoxRel(x, y, width + sb_width, hheight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+38,y+hheight+1, width-40, l_name, COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintIcon(iconfile, x + 8, y + 5);

	item_start_y = y+hheight;
	paintItems();
}

void CMenuWidget::paintItems()
{
	int item_height=height-(item_start_y-y);

	//Item not currently on screen
	if (selected >= 0)
	{
		while(selected < (int)page_start[current_page])
			current_page--;
		while(selected >= (int)page_start[current_page + 1])
			current_page++;
	}

	// Scrollbar
	if(total_pages>1)
	{
		frameBuffer->paintBoxRel(x+ width,item_start_y, 15, item_height, COL_MENUCONTENT_PLUS_1);
		frameBuffer->paintBoxRel(x+ width +2, item_start_y+ 2+ current_page*(item_height-4)/total_pages, 11, (item_height-4)/total_pages, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);
	}
	frameBuffer->paintBoxRel(x,item_start_y, width,item_height, COL_MENUCONTENT_PLUS_0);
	int ypos=item_start_y;
	for (unsigned int count = 0; count < items.size(); count++)
	{
		CMenuItem* item = items[count];

		if ((count >= page_start[current_page]) &&
		    (count < page_start[current_page + 1]))
		{
			item->init(x, ypos, width, iconOffset);
			if( (item->isSelectable()) && (selected==-1) )
			{
				ypos = item->paint(true);
				selected = count;
			}
			else
			{
				ypos = item->paint(selected==((signed int) count) );
			}
		}
		else
		{
			/* x = -1 is a marker which prevents the item from being painted on setActive changes */
			item->init(-1, 0, 0, 0);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------
CMenuOptionNumberChooser::CMenuOptionNumberChooser(const neutrino_locale_t name, int * const OptionValue, const bool Active, const int min_value, const int max_value, const int print_offset, const int special_value, const neutrino_locale_t special_value_name, const char * non_localized_name)
{
	height               = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName           = name;
	active               = Active;
	optionValue          = OptionValue;

	lower_bound          = min_value;
	upper_bound          = max_value;

	display_offset       = print_offset;

	localized_value      = special_value;
	localized_value_name = special_value_name;

	optionString         = non_localized_name;
}

int CMenuOptionNumberChooser::exec(CMenuTarget*)
{
	if (((*optionValue) >= upper_bound) || ((*optionValue) < lower_bound))
		*optionValue = lower_bound;
	else
		(*optionValue)++;

	paint(true);

	return menu_return::RETURN_NONE;
}

int CMenuOptionNumberChooser::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, RADIUS_SMALL);

	const char * l_option;
	char option_value[11];

	if ((localized_value_name == NONEXISTANT_LOCALE) || ((*optionValue) != localized_value))
	{
		sprintf(option_value, "%d", ((*optionValue) + display_offset));
		l_option = option_value;
	}
	else
		l_option = g_Locale->getText(localized_value_name);

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_option, true); // UTF-8
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + dx - stringwidth - 10; //+ offx

	const char * l_optionName = (optionString != NULL) ? optionString : g_Locale->getText(optionName);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), l_optionName, color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), l_option, color, 0, true); // UTF-8

	if (selected)
	{
		CLCD::getInstance()->showMenuText(0, l_optionName, -1, true); // UTF-8
		CLCD::getInstance()->showMenuText(1, l_option, -1, true); // UTF-8
	}

	return y+height;
}




CMenuOptionChooser::CMenuOptionChooser(const neutrino_locale_t OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName)
{
	height            = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionNameString  = g_Locale->getText(OptionName);
	optionName        = OptionName;
	options           = Options;
	active            = Active;
	optionValue       = OptionValue;
	number_of_options = Number_Of_Options;
	observ            = Observ;
	directKey         = DirectKey;
	iconName          = IconName;
}

CMenuOptionChooser::CMenuOptionChooser(const char* OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName)
{
	height            = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionNameString  = OptionName;
	optionName        = NONEXISTANT_LOCALE;
	options           = Options;
	active            = Active;
	optionValue       = OptionValue;
	number_of_options = Number_Of_Options;
	observ            = Observ;
	directKey         = DirectKey;
	iconName          = IconName;
}

void CMenuOptionChooser::setOptionValue(const int newvalue)
{
	*optionValue = newvalue;
}

int CMenuOptionChooser::getOptionValue(void) const
{
	return *optionValue;
}


int CMenuOptionChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;
	unsigned int count;

	for(count = 0; count < number_of_options; count++)
	{
		if (options[count].key == (*optionValue))
		{
			*optionValue = options[(count+1) % number_of_options].key;
			break;
		}
	}
	// if options are removed optionValue may not exist anymore -> use 1st available option
	if ((count == number_of_options) && number_of_options) {
		*optionValue = options[0].key;
	}

	paint(true);
	if(observ)
	{
		wantsRepaint = observ->changeNotify(optionName, optionValue);
	}
	if ( wantsRepaint )
		return menu_return::RETURN_REPAINT;
	else
		return menu_return::RETURN_NONE;
}

int CMenuOptionChooser::paint( bool selected )
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, RADIUS_SMALL);

	neutrino_locale_t option = NONEXISTANT_LOCALE;

	for(unsigned int count = 0 ; count < number_of_options; count++)
	{
		if (options[count].key == *optionValue)
		{
			option = options[count].value;
			break;
		}
	}

	if (!(iconName.empty()))
	{
		frameBuffer->paintIcon(iconName, x + 10, y + ((height - 20) >> 1));
	}
	else if (CRCInput::isNumeric(directKey))
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + 15, y+ height, height, CRCInput::getKeyName(directKey), color, height);
	}


	const char * l_option = g_Locale->getText(option);

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_option, true); // UTF-8
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + dx - stringwidth - 10; //+ offx

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), optionNameString.c_str(), color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), l_option, color, 0, true); // UTF-8

	if (selected)
	{
		CLCD::getInstance()->showMenuText(0, optionNameString.c_str(), -1, true); // UTF-8
		CLCD::getInstance()->showMenuText(1, l_option, -1, true); // UTF-8
	}

	return y+height;
}


//-------------------------------------------------------------------------------------------------------------------------------

CMenuOptionStringChooser::CMenuOptionStringChooser(const neutrino_locale_t OptionName, char* OptionValue, bool Active, CChangeObserver* Observ)
{
	height      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName  = OptionName;
	active      = Active;
	optionValue = OptionValue;
	observ      = Observ;

	directKey   = CRCInput::RC_nokey;
	iconName    = "";
}

void CMenuOptionStringChooser::removeOptions()
{
	options.clear();
}

CMenuOptionStringChooser::~CMenuOptionStringChooser()
{
	removeOptions();
}

void CMenuOptionStringChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

int CMenuOptionStringChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;
	//select next value
	for(unsigned int count = 0; count < options.size(); count++)
	{
		if ((strcmp(options[count].c_str(), optionValue) == 0) || (optionValue[0] == '\0'))
		{
			strcpy(optionValue, options[(count + 1) % options.size()].c_str());
			break;
		}
	}

	paint(true);
	if(observ)
	{
		wantsRepaint = observ->changeNotify(optionName, optionValue);
	}
	if ( wantsRepaint )
		return menu_return::RETURN_REPAINT;
	else
		return menu_return::RETURN_NONE;
}

int CMenuOptionStringChooser::paint( bool selected )
{
	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	CFrameBuffer::getInstance()->paintBoxRel(x, y, dx, height, bgcolor);

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(optionValue);
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + dx - stringwidth - 10; //+ offx

	const char * l_optionName = g_Locale->getText(optionName);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), l_optionName, color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), optionValue, color);

	if (selected)
	{
		CLCD::getInstance()->showMenuText(0, l_optionName, -1, true); // UTF-8
		CLCD::getInstance()->showMenuText(1, optionValue);
	}

	return y+height;
}


//-------------------------------------------------------------------------------------------------------------------------------

CMenuOptionLanguageChooser::CMenuOptionLanguageChooser(char* OptionValue, CChangeObserver* Observ)
{
	height      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionValue = OptionValue;
	observ      = Observ;

	directKey   = CRCInput::RC_nokey;
	iconName    = "";
}


CMenuOptionLanguageChooser::~CMenuOptionLanguageChooser()
{
	options.clear();
}

void CMenuOptionLanguageChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

int CMenuOptionLanguageChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;

	//select value
	for(unsigned int count = 0; count < options.size(); count++)
	{
		if (strcmp(options[count].c_str(), optionValue) == 0)
		{
			strcpy(g_settings.language, options[(count + 1) % options.size()].c_str());
			break;
		}
	}

	paint(true);
	if(observ)
	{
		wantsRepaint = observ->changeNotify(LOCALE_LANGUAGESETUP_SELECT, optionValue);
	}
	if ( wantsRepaint )
		return menu_return::RETURN_REPAINT;
	else
		return menu_return::RETURN_NONE;
}

int CMenuOptionLanguageChooser::paint( bool selected )
{
	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}

	CFrameBuffer::getInstance()->paintBoxRel(x, y, dx, height, bgcolor, RADIUS_SMALL);

	// 	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(optionValue);//unused variable
	int stringstartposOption = x + offx + 10;
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), optionValue, color);

	if (selected)
	{
		CLCD::getInstance()->showMenuText(1, optionValue);
	}

	return y+height;
}



//-------------------------------------------------------------------------------------------------------------------------------
CMenuForwarder::CMenuForwarder(const neutrino_locale_t Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = Option;
	option_string = NULL;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

CMenuForwarder::CMenuForwarder(const neutrino_locale_t Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = NULL;
	option_string = &Option;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

int CMenuForwarder::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuForwarder::exec(CMenuTarget* parent)
{
	if(jumpTarget)
	{
		return jumpTarget->exec(parent, actionKey);
	}
	else
	{
		return menu_return::RETURN_EXIT;
	}
}

const char * CMenuForwarder::getOption(void)
{
	if (option)
		return option;
	else
		if (option_string)
			return option_string->c_str();
		else
			return NULL;
}

const char * CMenuForwarder::getName(void)
{
	return g_Locale->getText(text);
}

int CMenuForwarder::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	int height = getHeight();
	const char * l_text = getName();

	int stringstartposX = x + offx + 10;

	const char * option_text = getOption();

	if (selected)
	{
		CLCD * lcd = CLCD::getInstance();
		lcd->showMenuText(0, l_text, -1, true); // UTF-8

		if (option_text != NULL)
			lcd->showMenuText(1, option_text);
		else
			lcd->showMenuText(1, "", -1, true); // UTF-8
	}

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, RADIUS_SMALL);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y+ height, dx- (stringstartposX - x), l_text, color, 0, true); // UTF-8

	if (!iconName.empty())
	{
		frameBuffer->paintIcon(iconName, x + 10, y+ ((height- 20)>>1) );
	}
	else if (CRCInput::isNumeric(directKey))
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + 15, y+ height, height, CRCInput::getKeyName(directKey), color, height);
	}

	if (option_text != NULL)
	{
		int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(option_text);
		int stringstartposOption = std::max(stringstartposX + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text) + 10,
											x + dx - stringwidth - 10); //+ offx

		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption- x),  option_text, color);
	}

	return y+ height;
}


//-------------------------------------------------------------------------------------------------------------------------------
const char * CMenuForwarderNonLocalized::getName(void)
{
	return the_text.c_str();
}

CMenuForwarderNonLocalized::CMenuForwarderNonLocalized(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName) : CMenuForwarder(NONEXISTANT_LOCALE, Active, Option, Target, ActionKey, DirectKey, IconName)
{
	the_text = Text;
}

CMenuForwarderNonLocalized::CMenuForwarderNonLocalized(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName) : CMenuForwarder(NONEXISTANT_LOCALE, Active, Option, Target, ActionKey, DirectKey, IconName)
{
    the_text = Text;
}

//-------------------------------------------------------------------------------------------------------------------------------
CMenuSeparator::CMenuSeparator(const int Type, const neutrino_locale_t Text)
{
	directKey = CRCInput::RC_nokey;
	iconName = "";
	type     = Type;
	text     = Text;
}


int CMenuSeparator::getHeight(void) const
{
	return (text == NONEXISTANT_LOCALE) ? 10 : g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

const char * CMenuSeparator::getString(void)
{
	return g_Locale->getText(text);
}

int CMenuSeparator::paint(bool selected)
{
	int height;
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	height = getHeight();
	uint8_t color, bgcolor0;
	
	if ((type & SUB_HEAD))
	{
		color = COL_MENUHEAD;
		bgcolor0 = COL_MENUHEAD_PLUS_0;
	}
	else
	{
		color = COL_MENUCONTENTINACTIVE;
		bgcolor0 = COL_MENUCONTENT_PLUS_0;
	}
		

	frameBuffer->paintBoxRel(x,y, dx, height, bgcolor0);
	if ((type & LINE))
	{
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1), COL_MENUCONTENT_PLUS_3);
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1)+1, COL_MENUCONTENT_PLUS_1);
	}
	if ((type & STRING))
	{

		if (text != NONEXISTANT_LOCALE)
		{
			int stringstartposX;

			const char * l_text = getString();
			int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

			/* if no alignment is specified, align centered */
			if (type & ALIGN_LEFT)
				stringstartposX = x + (!SUB_HEAD ?  20 : 20 +18);
			else if (type & ALIGN_RIGHT)
				stringstartposX = x + dx - stringwidth - 20;
			else /* ALIGN_CENTER */
				stringstartposX = x + (dx >> 1) - (stringwidth >> 1);

			frameBuffer->paintBoxRel(stringstartposX-5, y, stringwidth+10, height, bgcolor0);

			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y+height,dx- (stringstartposX- x) , l_text, color, 0, true); // UTF-8

			if (selected)
			{
				CLCD::getInstance()->showMenuText(0, l_text, -1, true); // UTF-8
				CLCD::getInstance()->showMenuText(1, "", -1, true); // UTF-8
			}
		}
	}
	return y+ height;
}

bool CPINProtection::check()
{
	char cPIN[4];
	neutrino_locale_t hint = NONEXISTANT_LOCALE;
	do
	{
		cPIN[0] = 0;
		CPINInput* PINInput = new CPINInput(LOCALE_PINPROTECTION_HEAD, cPIN, 4, hint);
		PINInput->exec( getParent(), "");
		delete PINInput;
		hint = LOCALE_PINPROTECTION_WRONGCODE;
	} while ((strncmp(cPIN,validPIN,4) != 0) && (cPIN[0] != 0));
	return ( strncmp(cPIN,validPIN,4) == 0);
}


bool CZapProtection::check()
{

	int res;
	char cPIN[5];
	neutrino_locale_t hint2 = NONEXISTANT_LOCALE;
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput(LOCALE_PARENTALLOCK_HEAD, cPIN, 4, hint2, fsk);

		res = PINInput->exec(getParent(), "");
		delete PINInput;

		hint2 = LOCALE_PINPROTECTION_WRONGCODE;
	} while ( (strncmp(cPIN,validPIN,4) != 0) &&
		  (cPIN[0] != 0) &&
		  ( res == menu_return::RETURN_REPAINT ) &&
		  ( fsk >= g_settings.parentallock_lockage ) );
	return ( ( strncmp(cPIN,validPIN,4) == 0 ) ||
			 ( fsk < g_settings.parentallock_lockage ) );
}

int CLockedMenuForwarder::exec(CMenuTarget* parent)
{
	Parent = parent;
	if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
		if (!check())
		{
			Parent = NULL;
			return menu_return::RETURN_REPAINT;
		}

	Parent = NULL;
	return CMenuForwarder::exec(parent);
}


int CMenuSelectorTarget::exec(CMenuTarget* /*parent*/, const std::string & actionKey)
{
//	printf("CMenuSelector: %s\n", actionKey.c_str());
	if (actionKey != "")
		*m_select = atoi(actionKey.c_str());
	else
		*m_select = -1;
	return menu_return::RETURN_EXIT;
}
