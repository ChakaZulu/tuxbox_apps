#include "scan.h"
#include "../global.h"

CScanTs::CScanTs()
{
	width = 400;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}


int CScanTs::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int key = g_RCInput->getKey(190);
	if(key != CRCInput::RC_ok)
	{
		hide();
		return CMenuTarget::RETURN_REPAINT;
	}

	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
	ypos+= hheight + (mheight >>1);
	
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.transponders").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.services").c_str(), COL_MENUCONTENT);
	g_RCInput->getKey(190);

	hide();
	return CMenuTarget::RETURN_REPAINT;
}

void CScanTs::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


void CScanTs::paint()
{
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("scants.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);
	
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.info1").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.info2").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

}
