#include "messagebox.h"
#include "../global.h"

CMessageBox::CMessageBox( string Caption, string Text, CMessageBoxNotifier* Notifier)
{
	width = 500;
	height = 150;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	caption = Caption;
	text = Text;
	notifier = Notifier;
	selected = 0;
}

void CMessageBox::paintHead()
{

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(caption), COL_MENUHEAD);

	g_FrameBuffer->paintBoxRel(x,y+theight+0, width,height - theight + 0, COL_MENUCONTENT);
	g_Fonts->menu_info->RenderString(x+10,y+theight+20+20, width, g_Locale->getText(text), COL_MENUCONTENT);

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
	if(selected==0)
		color = COL_MENUCONTENTSELECTED;
	g_FrameBuffer->paintBoxRel(xpos, y+height-fheight-20, ButtonWidth, fheight, color);
	g_FrameBuffer->paintIcon("gruen.raw", xpos+14, y+height-fheight-15);
	g_Fonts->infobar_small->RenderString(xpos + 43, y+height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.yes"), color);

	color = COL_INFOBAR_SHADOW;
	if(selected==1)
		color = COL_MENUCONTENTSELECTED;
	xpos = startpos+ButtonWidth+ButtonSpacing;
	g_FrameBuffer->paintBoxRel(xpos, y+height-fheight-20, ButtonWidth, fheight, color);
	g_FrameBuffer->paintIcon("rot.raw", xpos+14, y+height-fheight-15);
	g_Fonts->infobar_small->RenderString(xpos + 43, y+height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.no"), color);

	color = COL_INFOBAR_SHADOW;
	if(selected==2)
		color = COL_MENUCONTENTSELECTED;
	xpos = startpos+ButtonWidth*2+ButtonSpacing*2;
	g_FrameBuffer->paintBoxRel(xpos, y+height-fheight-20, ButtonWidth, fheight, color);
	g_FrameBuffer->paintIcon("home.raw", xpos+10, y+height-fheight-19);
	g_Fonts->infobar_small->RenderString(xpos + 43, y+height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.cancel"), color);
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
	result = mbrCancel;
}

void CMessageBox::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

int CMessageBox::exec(CMenuTarget* parent, string actionKey)
{
	if (parent)
	{
		parent->hide();
	}
	paintHead();
	paintButtons();
	bool loop=true;
	while (loop)
	{
		int key = g_RCInput->getKey();

		if ((key==CRCInput::RC_timeout) || (key==g_settings.key_channelList_cancel))
		{
			cancel();
			loop=false;
		}
		else if(key==CRCInput::RC_red)
		{
			no();
			loop=false;
		}
		else if(key==CRCInput::RC_green)
		{
			yes();
			loop=false;
		}
		else if(key==CRCInput::RC_right)
		{
			if(selected<2)
			{
				selected++;
				paintButtons();
			}

		}
		else if(key==CRCInput::RC_left)
		{
			if(selected>0)
			{
				selected--;
				paintButtons();
			}

		}
		else if(key==CRCInput::RC_left)
		{
		}
		else if(key==CRCInput::RC_ok)
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
		else
		{
			neutrino->HandleKeys( key );
		}

	}
	hide();
	return RETURN_REPAINT;
}

