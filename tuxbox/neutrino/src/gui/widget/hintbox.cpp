#include "hintbox.h"
#include "../global.h"

#define borderwidth 4

CHintBox::CHintBox( CMenuWidget* Parent, string Caption, string Text, int Width )
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
			text.insert( text.end(), Text.substr( 0, pos- 1 ) );
			Text= Text.substr( pos+ 1, uint(-1) );
		}
	} while ( ( pos != -1 ) );

	height = theight+ fheight* ( text.size()+ 1 );

	width = Width;
	for (int i= 0; i< text.size(); i++)
	{
		int nw= g_Fonts->menu->getRenderWidth( text[i].c_str() ) + 20;
		if ( nw> width )
			width= nw;
	}

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

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
	for (int i= 0; i< text.size(); i++)
		g_Fonts->menu->RenderString(x+10,y+ theight+ (fheight>>1)+ fheight* (i+ 1), width, text[i].c_str(), COL_MENUCONTENT);
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

