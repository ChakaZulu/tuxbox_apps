/*
$Id: menue.cpp,v 1.22 2001/10/22 21:48:22 McClean Exp $


History:
 $Log: menue.cpp,v $
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


int CMenuWidget::exec(CMenuTarget* parent, string)
{
    int pos;
    int key;


    if ( parent ) parent->hide();

    paint();
    int retval = CMenuItem::RETURN_REPAINT;

    do {
	key = g_RCInput->getKey(300);

	switch (key) {

		case (CRCInput::RC_up) :
		case (CRCInput::RC_down) :
			{
			   //search next / prev selectable item
			   for (unsigned int count=1; count< items.size(); count++) {

				if (key==CRCInput::RC_up) {
				    pos = selected- count;
				    if ( pos<0 )
					pos = items.size()-1;
				} else {
				    pos = (selected+ count)%items.size();
				}

				CMenuItem* item = items[pos];

				if ( item->isSelectable() ) {
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
		
		case (CRCInput::RC_ok) :
			{
				//exec this item...
				CMenuItem* item = items[selected];
				int ret = item->exec( this );
		
				if(ret==CMenuItem::RETURN_EXIT) {
					key = CRCInput::RC_timeout;
				} else if(ret==CMenuItem::RETURN_EXIT_ALL) {
					retval = CMenuItem::RETURN_EXIT_ALL;
					key = CRCInput::RC_timeout;
				} else if(ret==CMenuItem::RETURN_REPAINT) {
					paint();
				}
			}
			break;

		case (CRCInput::RC_home):
			key = CRCInput::RC_timeout;
			break;

		case (CRCInput::RC_left):
		case (CRCInput::RC_right):
			key = CRCInput::RC_timeout;
			break;

		case (CRCInput::RC_timeout):
			break;

		default:
			// unknown Key, push it back... and leave
			g_RCInput->pushbackKey (key);
			key = CRCInput::RC_timeout;
			break;
        }

   } while ( key!=CRCInput::RC_timeout );

   hide();
   if(!parent)
	   g_lcdd->setMode(LCDM_TV, g_Locale->getText(name));

   return retval;
}

void CMenuWidget::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height );
}

void CMenuWidget::paint()
{
    string  l_name = g_Locale->getText(name);
    g_lcdd->setMode(LCDM_MENU, l_name);

	x=((720-width)>>1) -20;
	y=(576-height)>>1;

	int hheight = g_Fonts->menu_title->getHeight();
	g_FrameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+38,y+hheight+1, width, l_name.c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintIcon(iconfile.c_str(),x+8,y+5);

	int ypos = y+hheight;

	for(unsigned int count=0;count<items.size();count++)
	{
		CMenuItem* item = items[count];
		item->init(x,ypos, width);
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

CMenuOptionChooser::CMenuOptionChooser(string OptionName, int* OptionValue, bool Active, CChangeObserver* Observ, bool Localizing)
{
	height= g_Fonts->menu->getHeight();
	optionName = OptionName;
	active = Active;
	optionValue = OptionValue;
	observ=Observ;
    localizing= Localizing;
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
		observ->changeNotify( optionName );
	}
	return 0;
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
	int stringstartposName = x + 10;
	int stringstartposOption = x + dx - stringwidth - 10;

	g_Fonts->menu->RenderString(stringstartposName,   y+height,dx,  l_optionName.c_str(), color);
	g_Fonts->menu->RenderString(stringstartposOption, y+height,dx,  l_option.c_str(), color);

    if(selected)
    {
        g_lcdd->setText(0, l_optionName);
        g_lcdd->setText(1, l_option);
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
		wantsRepaint = observ->changeNotify( optionName );
	}
    if ( wantsRepaint)
        return RETURN_REPAINT;
    else
        return 0;
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
	int stringstartposName = x + 10;
	int stringstartposOption = x + dx - stringwidth - 10;

	g_Fonts->menu->RenderString(stringstartposName,   y+height,dx, l_optionName.c_str(), color);
	g_Fonts->menu->RenderString(stringstartposOption, y+height,dx, l_option.c_str(), color);

    if(selected)
    {
        g_lcdd->setText(0, l_optionName);
        g_lcdd->setText(1, l_option);
    }

	return y+height;
}



//-------------------------------------------------------------------------------------------------------------------------------
CMenuForwarder::CMenuForwarder(string Text, bool Active, char* Option, CMenuTarget* Target, string ActionKey, bool Localizing)
{
	height=g_Fonts->menu->getHeight();
	text=Text;
	option = Option;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey;
    localizing = Localizing;
}

int CMenuForwarder::exec(CMenuTarget* parent)
{
	if(jumpTarget)
	{
		return jumpTarget->exec(parent, actionKey);
	}
	else
	{
		return RETURN_EXIT;
	}
}


int CMenuForwarder::paint(bool selected)
{
    string  l_text;

    if ( localizing )
        l_text = g_Locale->getText(text);
    else
        l_text = text;

	int stringstartposX = x+10;

        if(selected)
        {
            g_lcdd->setText(0, l_text);

    		if (option)
                g_lcdd->setText(1, option);
            else
                g_lcdd->setText(1, "");
        }

	unsigned char color = COL_MENUCONTENT;
	if (selected)
		color = COL_MENUCONTENTSELECTED;
	if (!active)
		color = COL_MENUCONTENTINACTIVE;

	g_FrameBuffer->paintBoxRel(x,y, dx, height, color );
	g_Fonts->menu->RenderString(stringstartposX, y+height,dx,  l_text.c_str(), color);

	if(option)
	{
		int stringwidth = g_Fonts->menu->getRenderWidth(option);
		int stringstartposOption = x + dx - stringwidth - 10;
		g_Fonts->menu->RenderString(stringstartposOption, y+height,dx,  option, color);
	}

	return y+ height;
}

//-------------------------------------------------------------------------------------------------------------------------------
CMenuSeparator::CMenuSeparator(int Type, string Text)
{
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

		g_Fonts->menu->RenderString(stringstartposX, y+height,dx, l_text.c_str(), COL_MENUCONTENT);

        if(selected)
        {
            g_lcdd->setText(0, l_text);
            g_lcdd->setText(1, "");
        }
	}
	return y+ height;
}


