#include "colorchooser.h"


CColorChooser::CColorChooser(string Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer)
{
	observer = Observer;
	name = Name;
	width = 300;
	height = 130;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	r = R;
	g = G;
	b = B;
	alpha = Alpha;
}

void CColorChooser::setColor(CFrameBuffer* frameBuffer, FontsDef *fonts)
{
	int color = convertSetupColor2RGB(*r,*g, *b);
	if(!alpha)
	{
		frameBuffer->paletteSetColor(254, color, 0);
		frameBuffer->paletteSet();
	}
	else
	{
		int tAlpha = convertSetupAlpha2Alpha( *alpha );
		frameBuffer->paletteSetColor(254, color, tAlpha);
		frameBuffer->paletteSet();
	}
	/*
	char colorstr[30];
	sprintf((char*)&colorstr, "%02x.%02x.%02x", *r, *g, *b);
	frameBuffer->paintBoxRel(x+218,y+107, 80, 20, COL_MENUCONTENT);
	fonts->epg_date->RenderString(x+218,y+120, 80, colorstr, COL_MENUCONTENT);
	*/
}

int CColorChooser::exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput,CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}
	setColor(frameBuffer,fonts);
	paint( frameBuffer, fonts );
	setColor(frameBuffer, fonts);

	bool loop = true;

	int selected = 0;
	while(loop)
	{
		int key = rcInput->getKey(300); 
		if(key==-1)
		{//timeout, close 
			loop = false;
		}
		else if (key==CRCInput::RC_ok)
		{
			loop=false;
		}
		else if (key==CRCInput::RC_down)
		{
			int max = 3;
			if (alpha==NULL)
			{
				max=2;
			}
			if(selected<max)
			{
				paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", false);
				paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", false);
				paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", false);
				paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",false);
				selected++;
				switch (selected)
				{
					case 0:
						paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", true);
						break;
					case 1:
						paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", true);
						break;
					case 2:
						paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", true);
						break;
					case 3:
						paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",true);
						break;
				}
			}
		}
		else if (key==CRCInput::RC_up)
		{
			if(selected>0)
			{
				paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", false);
				paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", false);
				paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", false);
				paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",false);
				selected--;
				switch (selected)
				{
					case 0:
						paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", true);
						break;
					case 1:
						paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", true);
						break;
					case 2:
						paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", true);
						break;
					case 3:
						paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",true);
						break;
				}
			}
		}
		else if (key==CRCInput::RC_right)
		{
			switch (selected)
			{
				case 0:
					if (*r<100)
					{
						*r+=5;
						paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", true);
						setColor(frameBuffer,fonts);
					}
					break;
				case 1:
					if (*g<100)
					{
						*g+=5;
						paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", true);
						setColor(frameBuffer,fonts);
					}
					break;
				case 2:
					if (*b<100)
					{
						*b+=5;
						paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", true);
						setColor(frameBuffer,fonts);
					}
					break;
				case 3:
					if (*alpha<100)
					{
						*alpha+=5;
						paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",true);
						setColor(frameBuffer,fonts);
					}
					break;
			}
		}
		else if (key==CRCInput::RC_left)
		{
			switch (selected)
			{
				case 0:
					if (*r>0)
					{
						*r-=5;
						paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", true);
						setColor(frameBuffer,fonts);
					}
					break;
				case 1:
					if (*g>0)
					{
						*g-=5;
						paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", true);
						setColor(frameBuffer,fonts);
					}
					break;
				case 2:
					if (*b>0)
					{
						*b-=5;
						paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", true);
						setColor(frameBuffer,fonts);
					}
					break;
				case 3:
					if (*alpha>0)
					{
						*alpha-=5;
						paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",true);
						setColor(frameBuffer,fonts);
					}
					break;
			}
		}
	}
	hide(frameBuffer);
	if(observer)
		observer->changeNotify(name);
	return CMenuTarget::RETURN_REPAINT;
}

void CColorChooser::hide(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,height, COL_BACKGROUND);
}

void CColorChooser::paint(CFrameBuffer* frameBuffer, FontsDef *fonts)
{

	frameBuffer->paintBoxRel(x,y, width,30, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+23, width, name.c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+30, width,height-30, COL_MENUCONTENT);

	paintSlider(frameBuffer, fonts, x+10, y+40, r,"red", true);
	paintSlider(frameBuffer, fonts, x+10, y+60, g,"green", false);
	paintSlider(frameBuffer, fonts, x+10, y+80, b,"blue", false);
	paintSlider(frameBuffer, fonts, x+10, y+100, alpha,"alpha",false);

	//color preview
	frameBuffer->paintBoxRel(x+220,y+40, 62,62, COL_MENUHEAD);
	frameBuffer->paintBoxRel(x+221,y+41, 60,60, 254);
}

void CColorChooser::paintSlider(CFrameBuffer* frameBuffer, FontsDef *fonts, int x, int y, unsigned char *spos, string text, bool selected)
{
	if (!spos)
		return;
	frameBuffer->paintBoxRel(x+70,y,120,16, COL_MENUCONTENT);
	frameBuffer->paintIcon("volumebody.raw",x+70,y+2);
	string iconfile = "volumeslider2";
	if (selected)
		iconfile += text;
	iconfile +=".raw";
	frameBuffer->paintIcon(iconfile,x+73+(*spos),y);

	fonts->menu->RenderString(x,y+13, width, text.c_str(), COL_MENUCONTENT);
}
