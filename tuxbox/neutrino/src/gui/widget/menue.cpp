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

/*
$Id: menue.cpp,v 1.41 2002/02/26 17:24:16 field Exp $


History:
 $Log: menue.cpp,v $
 Revision 1.41  2002/02/26 17:24:16  field
 Key-Handling weiter umgestellt EIN/AUS= KAPUTT!

 Revision 1.40  2002/02/25 19:32:26  field
 Events <-> Key-Handling umgestellt! SEHR BETA!

 Revision 1.39  2002/02/25 01:27:33  field
 Key-Handling umgestellt (moeglicherweise beta ;)

 Revision 1.38  2002/02/24 21:41:58  field
 User-Interface verbessert

 Revision 1.37  2002/02/23 14:31:07  field
 neue Icons

 Revision 1.34  2002/02/19 23:41:48  McClean
 add neutrino-direct-start option (for alexW's-Images only at the moment)

 Revision 1.33  2002/01/04 02:38:05  McClean
 cleanup

 Revision 1.32  2002/01/03 20:03:20  McClean
 cleanup

 Revision 1.31  2001/12/31 16:27:13  McClean
 use lcddclient

 Revision 1.30  2001/12/29 02:17:00  McClean
 make some settings get from controld

 Revision 1.29  2001/12/25 11:40:30  McClean
 better pushback handling

 Revision 1.28  2001/12/25 03:28:42  McClean
 better pushback-handling

 Revision 1.27  2001/12/12 19:11:32  McClean
 prepare timing setup...

 Revision 1.26  2001/11/26 02:34:04  McClean
 include (.../../stuff) changed - correct unix-formated files now

 Revision 1.25  2001/11/15 11:42:41  McClean
 gpl-headers added

 Revision 1.24  2001/11/07 23:48:55  field
 Kleiner Bugfix (Sprachenmenue)

 Revision 1.23  2001/11/03 23:23:51  McClean
 radiomode background paint - bugfix

 Revision 1.22  2001/10/22 21:48:22  McClean
 design-update

 Revision 1.21  2001/10/22 15:00:18  McClean
 icon update

 Revision 1.20  2001/10/15 00:24:07  McClean
 lcd-optimize

 Revision 1.19  2001/10/11 21:04:58  rasc
 - EPG:
   Event: 2 -zeilig: das passt aber noch nicht  ganz (read comments!).
   Key-handling etwas harmonischer gemacht  (Left/Right/Exit)
 - Code etwas restrukturiert und eine Fettnaepfe meinerseits beseitigt
   (\r\n wg. falscher CSV Einstellung...)

 Revision 1.18  2001/10/10 01:20:10  McClean
 menue changed

 Revision 1.17  2001/10/01 20:41:08  McClean
 plugin interface for games - beta but nice.. :)

 Revision 1.16  2001/09/23 21:34:07  rasc
 - LIFObuffer Module, pushbackKey fuer RCInput,
 - In einige Helper und widget-Module eingebracht
   ==> harmonischeres Menuehandling
 - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)


*/




#include "menue.h"
#include "../include/debug.h"
#include "../global.h"

CMenuWidget::CMenuWidget(string Name, string Icon, int mwidth, int mheight)
{
	onPaintNotifier = NULL;
	name = Name;
	iconfile = Icon;
	selected = -1;
	width = mwidth;
	height = mheight; // height(menu_title)+10+...
}

CMenuWidget::~CMenuWidget()
{
	for(unsigned int count=0;count<items.size();count++)
	{
		delete items[count];
	}
	items.clear();
}

void CMenuWidget::addItem(CMenuItem* menuItem, bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	items.insert(items.end(), menuItem);
}

void CMenuWidget::setOnPaintNotifier( COnPaintNotifier* nf )
{
	onPaintNotifier = nf;
}

int CMenuWidget::exec(CMenuTarget* parent, string)
{
	int pos;
	int i;

	if (parent)
		parent->hide();

	if (onPaintNotifier)
		onPaintNotifier->onPaintNotify(name);

	paint();
	int retval = menu_return::RETURN_REPAINT;
	int msg; uint data;

	do
	{

		g_RCInput->getMsg( &msg, &data, g_settings.timing_menu );

		int handled= false;

		for (i= 0; i< items.size(); i++)
		{
			CMenuItem* titem = items[i];
			if ( (titem->directKey!= -1) && (titem->directKey== msg) )
			{
				if (titem->isSelectable())
				{
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
								//clear prev. selected
								items[selected]->paint( false );
								//select new
								item->paint( true );
								selected = pos;
								break;
							}
						}
					}
					break;
				case (CRCInput::RC_ok):
					{
						//exec this item...
						CMenuItem* item = items[selected];

						switch ( item->exec( this ) )
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

				//pushback only these Keys
				case (CRCInput::RC_red):
				case (CRCInput::RC_green):
				case (CRCInput::RC_yellow):
				case (CRCInput::RC_blue):
					{
						g_RCInput->pushbackMsg( msg, data );
						msg = CRCInput::RC_timeout;
					}
					break;
				default:
					if ( neutrino->handleMsg( msg, data ) == messages_return::cancel_all )
					{
						retval = menu_return::RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
			}
		}

	}
	while ( msg!=CRCInput::RC_timeout );

	hide();
	if(!parent)
	{
		g_lcdd->setMode(CLcddClient::MODE_TVRADIO, g_Locale->getText(name));
	}

	return retval;
}

void CMenuWidget::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height );
}

void CMenuWidget::paint()
{
	string  l_name = g_Locale->getText(name);
	g_lcdd->setMode(CLcddClient::MODE_MENU, l_name);



	int neededWidth = g_Fonts->menu_title->getRenderWidth(l_name.c_str());
	if (neededWidth> width-48)
	{
		width= neededWidth+ 49;
	}

	iconOffset= 0;
	for (int i= 0; i< items.size(); i++)
	{
		if ( (items[i]->iconName!= "") ||
			 ((items[i]->directKey>= CRCInput::RC_0) && (items[i]->directKey<= CRCInput::RC_9)) )
		{
			iconOffset= g_Fonts->menu->getHeight();
			break;
		}
	}

	//	x=((720-width)>>1) -20;
	y=(576-height)>>1;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	//	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;


	int hheight = g_Fonts->menu_title->getHeight();
	g_FrameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+38,y+hheight+1, width, l_name.c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintIcon(iconfile.c_str(),x+8,y+5);

	int ypos = y+hheight;

	for(unsigned int count=0;count<items.size();count++)
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
	//	height = ypos - y;
}

//-------------------------------------------------------------------------------------------------------------------------------





CMenuOptionChooser::CMenuOptionChooser(string OptionName, int* OptionValue, bool Active, CChangeObserver* Observ, bool Localizing)
{
	height= g_Fonts->menu->getHeight();
	optionName = OptionName;
	active = Active;
	optionValue = OptionValue;
	observ=Observ;
	localizing= Localizing;
	directKey = -1;
	iconName = "";
}


CMenuOptionChooser::~CMenuOptionChooser()
{
	for(unsigned int count=0;count<options.size();count++)
	{
		delete options[count];
	}
	options.clear();
}

void CMenuOptionChooser::addOption(int key, string value)
{
	keyval *tmp = new keyval();
	tmp->key=key;
	tmp->value=value;
	options.insert(options.end(), tmp);
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
	unsigned char color = COL_MENUCONTENT;
	if (selected)
		color = COL_MENUCONTENTSELECTED;
	if (!active)
		color = COL_MENUCONTENTINACTIVE;

	g_FrameBuffer->paintBoxRel(x,y, dx, height, color );

	string option = "error";

	for(unsigned int count=0;count<options.size();count++)
	{
		keyval* kv = options[count];
		if(kv->key == *optionValue)
		{
			option = kv->value;
			break;
		}
	}

	string  l_optionName = g_Locale->getText(optionName);
	string  l_option;
	if ( localizing )
		l_option = g_Locale->getText(option);
	else
		l_option = option;

	int stringwidth = g_Fonts->menu->getRenderWidth(l_option.c_str());
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + offx + dx - stringwidth - 10;

	g_Fonts->menu->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), l_optionName.c_str(), color);
	g_Fonts->menu->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), l_option.c_str(), color);

	if(selected)
	{
		g_lcdd->setMenuText(0, l_optionName);
		g_lcdd->setMenuText(1, l_option);
	}

	return y+height;
}


//-------------------------------------------------------------------------------------------------------------------------------

CMenuOptionStringChooser::CMenuOptionStringChooser(string OptionName, char* OptionValue, bool Active, CChangeObserver* Observ, bool Localizing)
{
	height= g_Fonts->menu->getHeight();
	optionName = OptionName;
	active = Active;
	optionValue = OptionValue;
	observ=Observ;
	localizing= Localizing;

	directKey = -1;
	iconName = "";
}


CMenuOptionStringChooser::~CMenuOptionStringChooser()
{
	options.clear();
}

void CMenuOptionStringChooser::addOption( string value)
{
	options.insert(options.end(), value);
}

int CMenuOptionStringChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;
	//select next value
	for(unsigned int count=0;count<options.size();count++)
	{
		string actOption = options[count];
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
	unsigned char color = COL_MENUCONTENT;
	if (selected)
		color = COL_MENUCONTENTSELECTED;
	if (!active)
		color = COL_MENUCONTENTINACTIVE;

	g_FrameBuffer->paintBoxRel(x,y, dx, height, color );

	string  l_optionName = g_Locale->getText(optionName);
	string  l_option;
	if ( localizing )
		l_option = g_Locale->getText(optionValue);
	else
		l_option = optionValue;

	int stringwidth = g_Fonts->menu->getRenderWidth(l_option.c_str());
	int stringstartposName = x + offx + 10;
	int stringstartposOption = x + offx + dx - stringwidth - 10;

	g_Fonts->menu->RenderString(stringstartposName,   y+height,dx- (stringstartposName - x), l_optionName.c_str(), color);
	g_Fonts->menu->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), l_option.c_str(), color);

	if(selected)
	{
		g_lcdd->setMenuText(0, l_optionName);
		g_lcdd->setMenuText(1, l_option);
	}

	return y+height;
}



//-------------------------------------------------------------------------------------------------------------------------------
CMenuForwarder::CMenuForwarder(string Text, bool Active, char* Option, CMenuTarget* Target, string ActionKey, bool Localizing, int DirectKey, string IconName)
{
	height=g_Fonts->menu->getHeight();
	text=Text;
	option = Option;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey;
	localizing = Localizing;
	directKey = DirectKey;
	iconName = IconName;
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


int CMenuForwarder::paint(bool selected)
{
	string  l_text;

	if ( localizing )
		l_text = g_Locale->getText(text);
	else
		l_text = text;

	int stringstartposX = x + offx + 10;

	if(selected)
	{
		g_lcdd->setMenuText(0, l_text);

		if (option)
			g_lcdd->setMenuText(1, option);
		else
			g_lcdd->setMenuText(1, "");
	}

	unsigned char color = COL_MENUCONTENT;
	if (selected)
		color = COL_MENUCONTENTSELECTED;
	if (!active)
		color = COL_MENUCONTENTINACTIVE;

	g_FrameBuffer->paintBoxRel(x,y, dx, height, color );
	g_Fonts->menu->RenderString(stringstartposX, y+ height, dx- (stringstartposX - x),  l_text.c_str(), color);

	if (iconName!="")
	{
		g_FrameBuffer->paintIcon(iconName.c_str(), x + 10, y+ ((height- 20)>>1) );
	}
	else if ((directKey>= CRCInput::RC_0) && (directKey<= CRCInput::RC_9))
	{
		//number
		char tmp[10];
		sprintf((char*) tmp, "%d", directKey);

		g_Fonts->channellist_number->RenderString(x + 10, y+ height, height, tmp, color, height);
	}

	if(option)
	{
		int stringwidth = g_Fonts->menu->getRenderWidth(option);
		int stringstartposOption = x + offx + dx - stringwidth - 10;
		g_Fonts->menu->RenderString(stringstartposOption, y+height,dx- (stringstartposOption- x),  option, color);
	}

	return y+ height;
}

//-------------------------------------------------------------------------------------------------------------------------------
CMenuSeparator::CMenuSeparator(int Type, string Text)
{
	directKey = -1;
	iconName = "";

	height = g_Fonts->menu->getHeight();
	if(Text=="")
	{
		height = 10;
	}
	text = Text;

	if ( (Type & ALIGN_LEFT) || (Type & ALIGN_CENTER) || (Type & ALIGN_RIGHT) )
	{
		type=Type;
	}
	else
	{
		type= Type | ALIGN_CENTER;
	}
}


int CMenuSeparator::paint(bool selected)
{


	g_FrameBuffer->paintBoxRel(x,y, dx, height, COL_MENUCONTENT );
	if(type&LINE)
	{
		g_FrameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1), COL_MENUCONTENT+5 );
		g_FrameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1)+1, COL_MENUCONTENT+2 );
	}
	if(type&STRING)
	{
		string  l_text = g_Locale->getText(text);
		int stringwidth = g_Fonts->menu->getRenderWidth(l_text.c_str());
		int stringstartposX = 0;

		if(type&ALIGN_CENTER)
		{
			stringstartposX = (x + (dx >> 1)) - (stringwidth>>1);
		}
		else if(type&ALIGN_LEFT)
		{
			stringstartposX = x + 20;
		}
		else if(type&ALIGN_RIGHT)
		{
			stringstartposX = x + dx - stringwidth - 20;
		}

		g_FrameBuffer->paintBoxRel(stringstartposX-5, y, stringwidth+10, height, COL_MENUCONTENT );

		g_Fonts->menu->RenderString(stringstartposX, y+height,dx- (stringstartposX- x) , l_text.c_str(), COL_MENUCONTENT);

		if(selected)
		{
			g_lcdd->setMenuText(0, l_text);
			g_lcdd->setMenuText(1, "");
		}
	}
	return y+ height;
}


