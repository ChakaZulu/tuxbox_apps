#include "hintbox.h"
#include "../global.h"

#define borderwidth 4

CHintBox::CHintBox( CMenuWidget* Parent, string Caption, string Text, int Width )
{
	width = Width;
	height = 150;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	caption = Caption;
	text = Text;
	parent = Parent;
	pixbuf= NULL;
}

void CHintBox::paint( bool saveScreen = true )
{
	if (!saveScreen)
	{
		if (parent)
			parent->hide();
	}
	else
	{
		pixbuf= new unsigned char[(width+ 2* borderwidth) * (height+ 2* borderwidth)];
		if (pixbuf!= NULL)
			g_FrameBuffer->SaveScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);

		// clear border
		g_FrameBuffer->paintBackgroundBoxRel(x- borderwidth, y- borderwidth, width+ 2* borderwidth, borderwidth);
		g_FrameBuffer->paintBackgroundBoxRel(x- borderwidth, y+ height, width+ 2* borderwidth, borderwidth);
		g_FrameBuffer->paintBackgroundBoxRel(x- borderwidth, y, borderwidth, height);
		g_FrameBuffer->paintBackgroundBoxRel(x+ width, y, borderwidth, height);
	}

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(caption).c_str(), COL_MENUHEAD);

	g_FrameBuffer->paintBoxRel(x,y+theight+0, width,height - theight + 0, COL_MENUCONTENT);
	g_Fonts->menu_info->RenderString(x+10,y+theight+20+20, width, g_Locale->getText(text).c_str(), COL_MENUCONTENT);
}

void CHintBox::hide()
{
	if (pixbuf!= NULL)
	{
		g_FrameBuffer->RestoreScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);
		delete pixbuf;
		pixbuf= NULL;
	}
	else
		g_FrameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

