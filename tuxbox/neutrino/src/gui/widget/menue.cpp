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
CMenuForwarder CGenericMenuBack("menu.back");
CMenuSeparator * const GenericMenuSeparator = &CGenericMenuSeparator;
CMenuSeparator * const GenericMenuSeparatorLine = &CGenericMenuSeparatorLine;
CMenuForwarder * const GenericMenuBack = &CGenericMenuBack;




bool isNumber(const std::string& str)
{
	for (std::string::const_iterator i = str.begin(); i != str.end(); i++)
	{
		if (!std::isdigit(*i)) return false;
	}
	return true;
}


CMenuWidget::CMenuWidget(const char * const Name, const std::string & Icon, const int mwidth, const int mheight, const bool Localizing)
{
	frameBuffer = CFrameBuffer::getInstance();
	onPaintNotifier = NULL;
	name = Name;
	iconfile = Icon;
	selected = -1;
	width = mwidth;
	if(width > (g_settings.screen_EndX - g_settings.screen_StartX))
		width = g_settings.screen_EndX - g_settings.screen_StartX;
	height = mheight; // height(menu_title)+10+...
	wanted_height=mheight;
	localizing = Localizing;
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
	page_end.clear();
}

void CMenuWidget::addItem(CMenuItem* menuItem, const bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	items.push_back(menuItem);
}

void CMenuWidget::setOnPaintNotifier( COnPaintNotifier* nf )
{
	onPaintNotifier = nf;
}

int CMenuWidget::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int pos;

	if (parent)
		parent->hide();

	if (onPaintNotifier)
		onPaintNotifier->onPaintNotify(name);

	paint();
	int retval = menu_return::RETURN_REPAINT;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_menu );

	do
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);


		if ( msg <= CRCInput::RC_MaxRC )
		{
			timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_menu );
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
				case (CRCInput::RC_down) :
					{
						//search next / prev selectable item
						for (unsigned int count=1; count< items.size(); count++)
						{

							if (msg==CRCInput::RC_up)
							{
								pos = selected- count;
								if ( pos<0 )
									pos = items.size()-1;
							}
							else
							{
								pos = (selected+ count)%items.size();
							}

							CMenuItem* item = items[pos];

							if ( item->isSelectable() )
							{
								if(pos <= (int)page_end[current_page] && pos >= (int)page_start[current_page])
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
				// recalculate timeout für RC-Tasten
				timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_menu );
			}
		}

	}
	while ( msg!=CRCInput::RC_timeout );

	hide();
	if(!parent)
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	}

	return retval;
}

void CMenuWidget::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width+15,height );
}

void CMenuWidget::paint()
{
	std::string l_name = localizing ? g_Locale->getText(name) : name;

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, l_name.c_str());

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
	page_end.clear();
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
			page_end.push_back(i - 1);
			page_start.push_back(i);
			total_pages++;
			heightCurrPage=item_height;
		}
	}
	page_end.push_back(items.size() - 1);

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

	frameBuffer->paintBoxRel(x,y, width+sb_width,hheight, COL_MENUHEAD_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+38,y+hheight+1, width-40, l_name, COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintIcon(iconfile, x + 8, y + 5);

	item_start_y = y+hheight;
	paintItems();
}

void CMenuWidget::paintItems()
{
	int item_height=height-(item_start_y-y);
	
	//Item not currently on screen
	if(selected>=0)
	{
		while(selected < (int)page_start[current_page])
			current_page--;
		while(selected > (int)page_end[current_page])
			current_page++;
	}
	
	// Scrollbar
	if(total_pages>1)
	{
		frameBuffer->paintBoxRel(x+ width,item_start_y, 15, item_height, COL_MENUCONTENT_PLUS_1);
		float sbh= ((item_height-4) / total_pages);
		frameBuffer->paintBoxRel(x+ width +2, item_start_y+ 2+ int(current_page* sbh), 11, int(sbh), COL_MENUCONTENT_PLUS_3);
	}
	frameBuffer->paintBoxRel(x,item_start_y, width,item_height, COL_MENUCONTENT_PLUS_0);
	unsigned int count;
	int ypos=item_start_y;
	for(count=page_start[current_page]; count <= page_end[current_page];count++)
	{
		CMenuItem* item = items[count];
		item->init(x,ypos, width, iconOffset);
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
}

//-------------------------------------------------------------------------------------------------------------------------------





CMenuOptionChooser::CMenuOptionChooser(const char * const OptionName, int * const OptionValue, const bool Active, CChangeObserver * const Observ, const bool Localizing, const neutrino_msg_t DirectKey, const std::string & IconName)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName = OptionName;
	active = Active;
	optionValue = OptionValue;
	observ=Observ;
	localizing= Localizing;
	directKey = DirectKey;
	iconName = IconName;
}


CMenuOptionChooser::~CMenuOptionChooser()
{
	removeAllOptions();
}

void CMenuOptionChooser::addOption(const int key, const char * const value_utf8_encoded)
{
	keyval *tmp = new keyval();
	tmp->key=key;
	tmp->value = value_utf8_encoded;
	options.push_back(tmp);
}

void CMenuOptionChooser::removeAllOptions()
{
	for(unsigned int count=0;count<options.size();count++)
	{
		keyval* kv = options[count];
		delete kv;
	}
	options.clear();
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
	for(unsigned int count=0;count<options.size();count++)
	{
		keyval* kv = options[count];
		if(kv->key == *optionValue)
		{
			*optionValue = options[ (count+1)%options.size() ]->key;
			break;
		}
	}
	paint(true);
	if(observ)
	{
		observ->changeNotify( optionName, optionValue );
	}
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

	frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);

	std::string option = "error";

	for(unsigned int count=0;count<options.size();count++)
	{
		keyval* kv = options[count];
		if(kv->key == *optionValue)
		{
			option = kv->value;
			break;
		}
	}

	if (!(iconName.empty()))
	{
		frameBuffer->paintIcon(iconName, x + 10, y + ((height - 20) >> 1));
	}
	else if (CRCInput::isNumeric(directKey))
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + 10, y+ height, height, CRCInput::getKeyName(directKey), color, height);
	}


	const char * l_option;
	if (localizing && (!isNumber(option)))
		l_option = g_Locale->getText(option);
	else
		l_option = option.c_str();

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_option, true); // UTF-8
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + dx - stringwidth - 10; //+ offx

	const char * l_optionName = g_Locale->getText(optionName);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), l_optionName, color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), l_option, color, 0, true); // UTF-8

	if (selected)
	{
		CLCD::getInstance()->showMenuText(0, l_optionName, -1, true); // UTF-8
		CLCD::getInstance()->showMenuText(1, l_option, -1, true); // UTF-8
	}

	return y+height;
}


//-------------------------------------------------------------------------------------------------------------------------------

CMenuOptionStringChooser::CMenuOptionStringChooser(const char * const OptionName, char* OptionValue, bool Active, CChangeObserver* Observ, bool Localizing)
{
	height      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName  = OptionName;
	active      = Active;
	optionValue = OptionValue;
	observ      = Observ;
	localizing  = Localizing;

	directKey   = CRCInput::RC_nokey;
	iconName    = "";
}


CMenuOptionStringChooser::~CMenuOptionStringChooser()
{
	options.clear();
}

void CMenuOptionStringChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

int CMenuOptionStringChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;
	//select next value
	for(unsigned int count=0;count<options.size();count++)
	{
		std::string actOption = options[count];
		if(!strcmp( actOption.c_str(), optionValue))
		{
			strcpy(optionValue, options[ (count+1)%options.size() ].c_str());
			break;
		}
	}

	paint(true);
	if(observ)
	{
		wantsRepaint = observ->changeNotify( optionName, optionValue );
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

	const char * l_option = localizing ? g_Locale->getText(optionValue) : optionValue;

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_option);
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + dx - stringwidth - 10; //+ offx

	const char * l_optionName = g_Locale->getText(optionName);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), l_optionName, color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), l_option, color);

	if (selected)
	{
		CLCD::getInstance()->showMenuText(0, l_optionName, -1, true); // UTF-8
		CLCD::getInstance()->showMenuText(1, l_option);
	}

	return y+height;
}



//-------------------------------------------------------------------------------------------------------------------------------
CMenuForwarder::CMenuForwarder(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, const bool Localizing, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = Option;
	option_string = NULL;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	localizing = Localizing;
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

CMenuForwarder::CMenuForwarder(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, const bool Localizing, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = NULL;
	option_string = &Option;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	localizing = Localizing;
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

int CMenuForwarder::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	int height = getHeight();
	const char * l_text = localizing ? g_Locale->getText(text) : text.c_str();

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

	frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y+ height, dx- (stringstartposX - x), l_text, color, 0, true); // UTF-8

	if (!iconName.empty())
	{
		frameBuffer->paintIcon(iconName, x + 10, y+ ((height- 20)>>1) );
	}
	else if (CRCInput::isNumeric(directKey))
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + 10, y+ height, height, CRCInput::getKeyName(directKey), color, height);
	}

	if (option_text != NULL)
	{
		int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(option_text);
		int stringstartposOption = x + dx - stringwidth - 10; //+ offx

		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption- x),  option_text, color);
	}

	return y+ height;
}

//-------------------------------------------------------------------------------------------------------------------------------
CMenuSeparator::CMenuSeparator(const int Type, const char * const Text)
{
	directKey = CRCInput::RC_nokey;
	iconName = "";
	type = Type;

	text = Text ? Text : "";
}


int CMenuSeparator::getHeight(void) const
{
	return text.empty() ? 10 : g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuSeparator::paint(bool selected)
{
	int height;
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	height = getHeight();
	
	frameBuffer->paintBoxRel(x,y, dx, height, COL_MENUCONTENT_PLUS_0);
	if(type&LINE)
	{
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1), COL_MENUCONTENT_PLUS_3);
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1)+1, COL_MENUCONTENT_PLUS_1);
	}
	if(type&STRING)
	{
		int stringstartposX;
		const char * l_text = g_Locale->getText(text);
		int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

		/* if no alignment is specified, align centered */
		if (type & ALIGN_LEFT)
			stringstartposX = x + 20;
		else if (type & ALIGN_RIGHT)
			stringstartposX = x + dx - stringwidth - 20;
		else /* ALIGN_CENTER */
			stringstartposX = x + (dx >> 1) - (stringwidth >> 1);

		frameBuffer->paintBoxRel(stringstartposX-5, y, stringwidth+10, height, COL_MENUCONTENT_PLUS_0);

		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y+height,dx- (stringstartposX- x) , l_text, COL_MENUCONTENTINACTIVE, 0, true); // UTF-8

		if(selected)
		{
			CLCD::getInstance()->showMenuText(0, l_text, -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, "", -1, true); // UTF-8
		}
	}
	return y+ height;
}

bool CPINProtection::check()
{
	char cPIN[4];
	const char * hint = NULL;
	do
	{
		cPIN[0] = 0;
		CPINInput* PINInput = new CPINInput( "pinprotection.head", cPIN, 4, hint);
		PINInput->exec( getParent(), "");
		delete PINInput;
		hint = "pinprotection.wrongcode";
	} while ((strncmp(cPIN,validPIN,4) != 0) && (cPIN[0] != 0));
	return ( strncmp(cPIN,validPIN,4) == 0);
}


bool CZapProtection::check()
{

	int res;
	char cPIN[5];
	const char * hint2 = NULL;
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput( "parentallock.head", cPIN, 4, hint2, fsk );

		res = PINInput->exec( getParent(), "");
		delete PINInput;

		hint2 = "pinprotection.wrongcode";
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
