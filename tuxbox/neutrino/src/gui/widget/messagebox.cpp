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
}

void CMessageBox::paint()
{
	int ButtonWidth = width / 3;

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(caption).c_str(), COL_MENUHEAD);

	g_FrameBuffer->paintBoxRel(x,y+theight+0, width,height - theight + 0, COL_MENUCONTENT);
	g_Fonts->menu_info->RenderString(x+10,y+theight+0+20, width, g_Locale->getText(text).c_str(), COL_MENUCONTENT);

	g_FrameBuffer->paintHLine(x, x+width, y+height-fheight, COL_INFOBAR_SHADOW);
	g_FrameBuffer->paintBoxRel(x,y+height-fheight, width,fheight+0, COL_MENUHEAD);

	g_FrameBuffer->paintIcon("rot.raw", x+width- 3* ButtonWidth+ 8, y+height+4-fheight);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth+ 29, y+height+24-fheight - 2, ButtonWidth- 26, g_Locale->getText("messagebox.yes").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("rot.raw", x+width- 2* ButtonWidth+ 8, y+height+4-fheight);
	g_Fonts->infobar_small->RenderString(x+width- 2* ButtonWidth+ 29, y+height+24-fheight - 2, ButtonWidth- 26, g_Locale->getText("messagebox.no").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("rot.raw", x+width- ButtonWidth+ 8, y+height+4-fheight);
	g_Fonts->infobar_small->RenderString(x+width- ButtonWidth+ 29, y+height+24-fheight - 2, ButtonWidth- 26, g_Locale->getText("messagebox.cancel").c_str(), COL_INFOBAR);

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
	paint();
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
		else if(key==CRCInput::RC_ok)
		{
			yes();
			loop=false;
		}

		else if( (key==CRCInput::RC_spkr) || (key==CRCInput::RC_plus) || (key==CRCInput::RC_minus)
		         || (key==CRCInput::RC_standby)
		         || (CRCInput::isNumeric(key)) )
		{
			cancel();
			g_RCInput->pushbackKey (key);
			loop=false;
		}
	}
	hide();
	return RETURN_REPAINT;
}

