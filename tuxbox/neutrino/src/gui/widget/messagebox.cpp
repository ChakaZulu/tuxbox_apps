#include "messagebox.h"
#include "../global.h"

#define borderwidth 4

CMessageBox::CMessageBox( string Caption, string Text, CMessageBoxNotifier* Notifier, int Width, uint Default, uint ShowButtons )
{
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->menu->getHeight();

	caption = Caption;
	Text = Text+ "\n";
	text.clear();

	int pos;
	do
	{
		pos = Text.find_first_of("\n");
		if ( pos!=-1 )
		{
			text.insert( text.end(), Text.substr( 0, pos ) );
			Text= Text.substr( pos+ 1, uint(-1) );
		}
	} while ( ( pos != -1 ) );
	height = theight+ fheight* ( text.size()+ 3 );

	width = Width;
	if ( width< 450 )
		width = 450;
	for (int i= 0; i< text.size(); i++)
	{
		int nw= g_Fonts->menu->getRenderWidth( text[i].c_str() ) + 20;
		if ( nw> width )
			width= nw;
	}

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	notifier = Notifier;
	switch (Default)
	{
		case mbrYes:
			selected = 0;
			break;
		case mbrNo:
			selected = 1;
			break;
		case mbrCancel:
			selected = 2;
			break;
		case mbrBack:
			selected = 2;
			break;
	}
	showbuttons= ShowButtons;
}

void CMessageBox::paintHead()
{

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(caption), COL_MENUHEAD);

	g_FrameBuffer->paintBoxRel(x,y+theight+0, width,height - theight + 0, COL_MENUCONTENT);
	for (int i= 0; i< text.size(); i++)
		g_Fonts->menu->RenderString(x+10,y+ theight+ (fheight>>1)+ fheight* (i+ 1), width, text[i].c_str(), COL_MENUCONTENT);

}

void CMessageBox::paintButtons()
{
	//irgendwann alle vergleichen - aber cancel ist sicher der längste
	int MaxButtonTextWidth = g_Fonts->infobar_small->getRenderWidth(g_Locale->getText("messagebox.cancel").c_str());

	int ButtonSpacing = 40;
	int ButtonWidth = 20 + 33 + MaxButtonTextWidth;
	int startpos = x + (width - ((ButtonWidth*3)+(ButtonSpacing*2))) / 2;

	int xpos = startpos;
	int color = COL_INFOBAR_SHADOW;

	if ( showbuttons & mbYes )
	{
		if(selected==0)
			color = COL_MENUCONTENTSELECTED;
		g_FrameBuffer->paintBoxRel(xpos, y+height-fheight-20, ButtonWidth, fheight, color);
		g_FrameBuffer->paintIcon("rot.raw", xpos+14, y+height-fheight-15);
		g_Fonts->infobar_small->RenderString(xpos + 43, y+height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.yes"), color);
	}

	xpos = startpos+ButtonWidth+ButtonSpacing;

	if ( showbuttons & mbNo )
	{
		color = COL_INFOBAR_SHADOW;
		if(selected==1)
			color = COL_MENUCONTENTSELECTED;

		g_FrameBuffer->paintBoxRel(xpos, y+height-fheight-20, ButtonWidth, fheight, color);
		g_FrameBuffer->paintIcon("gruen.raw", xpos+14, y+height-fheight-15);
		g_Fonts->infobar_small->RenderString(xpos + 43, y+height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.no"), color);
    }

    xpos = startpos+ButtonWidth*2+ButtonSpacing*2;
    if ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) )
	{
		color = COL_INFOBAR_SHADOW;
		if(selected==2)
			color = COL_MENUCONTENTSELECTED;

		g_FrameBuffer->paintBoxRel(xpos, y+height-fheight-20, ButtonWidth, fheight, color);
		g_FrameBuffer->paintIcon("home.raw", xpos+10, y+height-fheight-19);
		g_Fonts->infobar_small->RenderString(xpos + 43, y+height-fheight+4, ButtonWidth- 53, g_Locale->getText( ( showbuttons & mbCancel ) ? "messagebox.cancel" : "messagebox.back" ), color);
	}
}

void CMessageBox::yes()
{
	result = mbrYes;
	if (notifier)
		notifier->onYes();
}

void CMessageBox::no()
{
	result = mbrNo;
	if (notifier)
		notifier->onNo();
}

void CMessageBox::cancel()
{
	if ( showbuttons & mbCancel )
		result = mbrCancel;
	else
		result = mbrBack;
}

void CMessageBox::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

int CMessageBox::exec(CMenuTarget* parent, string actionKey)
{

	int res = menu_return::RETURN_REPAINT;
    unsigned char pixbuf[(width+ 2* borderwidth) * (height+ 2* borderwidth)];
	g_FrameBuffer->SaveScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);

	// clear border
	g_FrameBuffer->paintBackgroundBoxRel(x- borderwidth, y- borderwidth, width+ 2* borderwidth, borderwidth);
	g_FrameBuffer->paintBackgroundBoxRel(x- borderwidth, y+ height, width+ 2* borderwidth, borderwidth);
	g_FrameBuffer->paintBackgroundBoxRel(x- borderwidth, y, borderwidth, height);
	g_FrameBuffer->paintBackgroundBoxRel(x+ width, y, borderwidth, height);

	paintHead();
	paintButtons();

	bool loop=true;
	while (loop)
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, g_settings.timing_epg );

		if ( ( (msg==CRCInput::RC_timeout) ||
			   (msg==g_settings.key_channelList_cancel) ) &&
			 ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) ) )
		{
			cancel();
			loop=false;
		}
		else if ( (msg==CRCInput::RC_green) && ( showbuttons & mbNo ) )
		{
			no();
			loop=false;
		}
		else if ( (msg==CRCInput::RC_red) && ( showbuttons & mbYes ) )
		{
			yes();
			loop=false;
		}
		else if(msg==CRCInput::RC_right)
		{
			bool ok = false;
			while (!ok)
			{
				selected++;
				switch (selected)
				{
					case 3:
						selected= -1;
					    break;
					case 0:
						ok = ( showbuttons & mbYes );
						break;
					case 1:
						ok = ( showbuttons & mbNo );
						break;
					case 2:
						ok = ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) );
						break;
				}
			}

			paintButtons();
		}
		else if(msg==CRCInput::RC_left)
		{
			bool ok = false;
			while (!ok)
			{
				selected--;
				switch (selected)
				{
					case -1:
						selected= 3;
					    break;
					case 0:
						ok = ( showbuttons & mbYes );
						break;
					case 1:
						ok = ( showbuttons & mbNo );
						break;
					case 2:
						ok = ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) );
						break;
				}
			}

			paintButtons();

		}
		else if(msg==CRCInput::RC_ok)
		{
			//exec selected;
			switch (selected)
			{
				case 0: yes();
					break;
				case 1: no();
					break;
				case 2: cancel();
					break;
			}
			loop=false;
		}
		else if ( neutrino->handleMsg( msg, data ) == messages_return::cancel_all )
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}

	}

	g_FrameBuffer->RestoreScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);
	return res;
}

int ShowMsg ( string Caption, string Text, uint Default, uint ShowButtons )
{
   	CMessageBox* messageBox = new CMessageBox( Caption, Text, NULL, 450, Default, ShowButtons );
	messageBox->exec( NULL, "");
	int res= messageBox->result;
	delete messageBox;

	return res;
}

