#include "screensetup.h"
#include "../global.h"

int CScreenSetup::exec( CMenuTarget* parent, string )
{
	if (parent)
	{
		parent->hide();
	}
	
	paint();

	bool loop = true;
	selected = 1;
	while(loop)
	{
		int key = g_RCInput->getKey(300);
		if (key==CRCInput::RC_timeout)
		{//timeout, close 
			loop = false;
		}
		else if (key==CRCInput::RC_ok)
		{
			loop=false;
		}
		else if (key==CRCInput::RC_red)
		{
			selected = 1;
		}
		else if (key==CRCInput::RC_green)
		{
			selected = 2;
		}


		if(selected==1)
		{//upper left setup
			if (key==CRCInput::RC_up)
			{
				g_settings.screen_StartY--;
				if(g_settings.screen_StartY<0)
				{
					g_settings.screen_StartY=0;
				}
				else
					paintBorderUL();
			}
			else if (key==CRCInput::RC_down)
			{
				g_settings.screen_StartY++;
				if(g_settings.screen_StartY>200)
				{
					g_settings.screen_StartY=200;
				}
				else
					paintBorderUL();
			}
			else if (key==CRCInput::RC_left)
			{
				g_settings.screen_StartX--;
				if(g_settings.screen_StartX<0)
				{
					g_settings.screen_StartX=0;
				}
				else
					paintBorderUL();
			}
			else if (key==CRCInput::RC_right)
			{
				g_settings.screen_StartX++;
				if(g_settings.screen_StartX>200)
				{
					g_settings.screen_StartX=200;
				}
				else
					paintBorderUL();
			}
		}
		else if(selected==2)
		{//upper left setup
			if (key==CRCInput::RC_up)
			{
				g_settings.screen_EndY--;
				if(g_settings.screen_EndY<400)
				{
					g_settings.screen_EndY=400;
				}
				else
					paintBorderLR();
			}
			else if (key==CRCInput::RC_down)
			{
				g_settings.screen_EndY++;
				if(g_settings.screen_EndY>575)
				{
					g_settings.screen_EndY=575;
				}
				else
					paintBorderLR();
			}
			else if (key==CRCInput::RC_left)
			{
				g_settings.screen_EndX--;
				if(g_settings.screen_EndX<400)
				{
					g_settings.screen_EndX=400;
				}
				else
					paintBorderLR();
			}
			else if (key==CRCInput::RC_right)
			{
				g_settings.screen_EndX++;
				if(g_settings.screen_EndX>719)
				{
					g_settings.screen_EndX=719;
				}
				else
					paintBorderLR();
			}
		}
	}
	
	hide();
	return CMenuTarget::RETURN_REPAINT;
}

void CScreenSetup::hide()
{
	memset(g_FrameBuffer->lfb, COL_BACKGROUND, g_FrameBuffer->Stride()*576);
}

void CScreenSetup::paintBorderUL()
{
	g_FrameBuffer->paintIcon("border_ul.raw", g_settings.screen_StartX, g_settings.screen_StartY);
	int x=15*19;
	int y=15*16;
	g_FrameBuffer->paintBoxRel(x,y, 15*9,15*6, COL_MENUHEAD);
	char xpos[30];
	char ypos[30];
	char xepos[30];
	char yepos[30];

	sprintf((char*) &xpos, "SX: %d", g_settings.screen_StartX);
	sprintf((char*) &ypos, "SY: %d", g_settings.screen_StartY);
	sprintf((char*) &xepos, "EX: %d", g_settings.screen_EndX);
	sprintf((char*) &yepos, "EY: %d", g_settings.screen_EndY);

	g_Fonts->fixedabr20->RenderString(x+10,y+30, 200, xpos, COL_MENUHEAD);
	g_Fonts->fixedabr20->RenderString(x+10,y+50, 200, ypos, COL_MENUHEAD);
	g_Fonts->fixedabr20->RenderString(x+10,y+70, 200, xepos, COL_MENUHEAD);
	g_Fonts->fixedabr20->RenderString(x+10,y+90, 200, yepos, COL_MENUHEAD);
}

void CScreenSetup::paintBorderLR()
{
	g_FrameBuffer->paintIcon("border_lr.raw", g_settings.screen_EndX-96, g_settings.screen_EndY-96);
	int x=15*19;
	int y=15*16;
	g_FrameBuffer->paintBoxRel(x,y, 15*9,15*6, COL_MENUHEAD);
	char xpos[30];
	char ypos[30];
	char xepos[30];
	char yepos[30];

	sprintf((char*) &xpos, "SX: %d", g_settings.screen_StartX);
	sprintf((char*) &ypos, "SY: %d", g_settings.screen_StartY);
	sprintf((char*) &xepos, "EX: %d", g_settings.screen_EndX);
	sprintf((char*) &yepos, "EY: %d", g_settings.screen_EndY);

	g_Fonts->fixedabr20->RenderString(x+10,y+30, 200, xpos, COL_MENUHEAD);
	g_Fonts->fixedabr20->RenderString(x+10,y+50, 200, ypos, COL_MENUHEAD);
	g_Fonts->fixedabr20->RenderString(x+10,y+70, 200, xepos, COL_MENUHEAD);
	g_Fonts->fixedabr20->RenderString(x+10,y+90, 200, yepos, COL_MENUHEAD);
}

void CScreenSetup::paint()
{
	memset(g_FrameBuffer->lfb, 7, g_FrameBuffer->Stride()*576);

	for(int count=0;count<576;count+=15)
		g_FrameBuffer->paintHLine(0,719, count, 8 );

	for(int count=0;count<720;count+=15)
		g_FrameBuffer->paintVLine(count,0, 575, 8 );

	g_FrameBuffer->paintBox(0,0, 15*15,15*15, 7);
	g_FrameBuffer->paintBox(32*15+1,23*15+1, 719,575, 7);

	int x=15*5;
	int y=15*24;
	g_FrameBuffer->paintBoxRel(x,y, 15*23,15*4, COL_MENUHEAD);
	
	g_Fonts->menu->RenderString(x+30,y+26, 15*23, "red = setup upper left", COL_MENUHEAD);
	g_Fonts->menu->RenderString(x+30,y+46, 15*23, "green = setup lower right", COL_MENUHEAD);


	paintBorderUL();
	paintBorderLR();
}


