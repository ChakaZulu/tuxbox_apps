#include "screensetup.h"


CScreenSetup::CScreenSetup(string Name, FontsDef *Fonts, SNeutrinoSettings* Settings)
{
	name = Name;
	settings = Settings;
	fonts = Fonts;
}


int CScreenSetup::exec(CFrameBuffer* frameBuffer, CRCInput *rcInput,CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}
	
	paint( frameBuffer );

	bool loop = true;
	selected = 1;
	while(loop)
	{
		int key = rcInput->getKey(300); 
		if(key==CRCInput::RC_timeout)
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
				settings->screen_StartY--;
				if(settings->screen_StartY<0)
				{
					settings->screen_StartY=0;
				}
				else
					paintBorderUL( frameBuffer);
			}
			else if (key==CRCInput::RC_down)
			{
				settings->screen_StartY++;
				if(settings->screen_StartY>200)
				{
					settings->screen_StartY=200;
				}
				else
					paintBorderUL( frameBuffer );
			}
			else if (key==CRCInput::RC_left)
			{
				settings->screen_StartX--;
				if(settings->screen_StartX<0)
				{
					settings->screen_StartX=0;
				}
				else
					paintBorderUL( frameBuffer );
			}
			else if (key==CRCInput::RC_right)
			{
				settings->screen_StartX++;
				if(settings->screen_StartX>200)
				{
					settings->screen_StartX=200;
				}
				else
					paintBorderUL( frameBuffer );
			}
		}
		else if(selected==2)
		{//upper left setup
			if (key==CRCInput::RC_up)
			{
				settings->screen_EndY--;
				if(settings->screen_EndY<400)
				{
					settings->screen_EndY=400;
				}
				else
					paintBorderLR( frameBuffer );
			}
			else if (key==CRCInput::RC_down)
			{
				settings->screen_EndY++;
				if(settings->screen_EndY>575)
				{
					settings->screen_EndY=575;
				}
				else
					paintBorderLR( frameBuffer );
			}
			else if (key==CRCInput::RC_left)
			{
				settings->screen_EndX--;
				if(settings->screen_EndX<400)
				{
					settings->screen_EndX=400;
				}
				else
					paintBorderLR( frameBuffer );
			}
			else if (key==CRCInput::RC_right)
			{
				settings->screen_EndX++;
				if(settings->screen_EndX>719)
				{
					settings->screen_EndX=719;
				}
				else
					paintBorderLR( frameBuffer );
			}
		}
	}
	
	hide(frameBuffer);
	return CMenuTarget::RETURN_REPAINT;
}

void CScreenSetup::hide(CFrameBuffer* frameBuffer)
{
	memset(frameBuffer->lfb, COL_BACKGROUND, frameBuffer->Stride()*576);
}

void CScreenSetup::paintBorderUL(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintIcon("border_ul.raw", settings->screen_StartX, settings->screen_StartY);
	int x=15*19;
	int y=15*16;
	frameBuffer->paintBoxRel(x,y, 15*9,15*6, COL_MENUHEAD);
	char xpos[30];
	char ypos[30];
	char xepos[30];
	char yepos[30];

	sprintf((char*) &xpos, "SX: %d", settings->screen_StartX);
	sprintf((char*) &ypos, "SY: %d", settings->screen_StartY);
	sprintf((char*) &xepos, "EX: %d", settings->screen_EndX);
	sprintf((char*) &yepos, "EY: %d", settings->screen_EndY);

	fonts->fixedabr20->RenderString(x+10,y+30, 200, xpos, COL_MENUHEAD);
	fonts->fixedabr20->RenderString(x+10,y+50, 200, ypos, COL_MENUHEAD);
	fonts->fixedabr20->RenderString(x+10,y+70, 200, xepos, COL_MENUHEAD);
	fonts->fixedabr20->RenderString(x+10,y+90, 200, yepos, COL_MENUHEAD);
}

void CScreenSetup::paintBorderLR(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintIcon("border_lr.raw", settings->screen_EndX-96, settings->screen_EndY-96);
	int x=15*19;
	int y=15*16;
	frameBuffer->paintBoxRel(x,y, 15*9,15*6, COL_MENUHEAD);
	char xpos[30];
	char ypos[30];
	char xepos[30];
	char yepos[30];

	sprintf((char*) &xpos, "SX: %d", settings->screen_StartX);
	sprintf((char*) &ypos, "SY: %d", settings->screen_StartY);
	sprintf((char*) &xepos, "EX: %d", settings->screen_EndX);
	sprintf((char*) &yepos, "EY: %d", settings->screen_EndY);

	fonts->fixedabr20->RenderString(x+10,y+30, 200, xpos, COL_MENUHEAD);
	fonts->fixedabr20->RenderString(x+10,y+50, 200, ypos, COL_MENUHEAD);
	fonts->fixedabr20->RenderString(x+10,y+70, 200, xepos, COL_MENUHEAD);
	fonts->fixedabr20->RenderString(x+10,y+90, 200, yepos, COL_MENUHEAD);
}

void CScreenSetup::paint(CFrameBuffer* frameBuffer)
{
	memset(frameBuffer->lfb, 7, frameBuffer->Stride()*576);

	for(int count=0;count<576;count+=15)
		frameBuffer->paintHLine(0,719, count, 8 );

	for(int count=0;count<720;count+=15)
		frameBuffer->paintVLine(count,0, 575, 8 );

	frameBuffer->paintBox(0,0, 15*15,15*15, 7);
	frameBuffer->paintBox(32*15+1,23*15+1, 719,575, 7);

	int x=15*5;
	int y=15*24;
	frameBuffer->paintBoxRel(x,y, 15*23,15*4, COL_MENUHEAD);
	
	fonts->menu->RenderString(x+30,y+26, 15*23, "red = setup upper left", COL_MENUHEAD);
	fonts->menu->RenderString(x+30,y+46, 15*23, "green = setup lower right", COL_MENUHEAD);


	paintBorderUL( frameBuffer );
	paintBorderLR( frameBuffer );
}

