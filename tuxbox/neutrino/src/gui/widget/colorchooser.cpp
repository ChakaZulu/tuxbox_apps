#include "colorchooser.h"
#include "../global.h"

CColorChooser::CColorChooser(string Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer)
{
	observer = Observer;
	name = Name;
	width = 360;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+ mheight* 4;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	r = R;
	g = G;
	b = B;
	alpha = Alpha;
}

void CColorChooser::setColor()
{
	int color = convertSetupColor2RGB(*r,*g, *b);
	if(!alpha)
	{
		g_FrameBuffer->paletteSetColor(254, color, 0);
		g_FrameBuffer->paletteSet();
	}
	else
	{
		int tAlpha = convertSetupAlpha2Alpha( *alpha );
		g_FrameBuffer->paletteSetColor(254, color, tAlpha);
		g_FrameBuffer->paletteSet();
	}
	/*
	char colorstr[30];
	sprintf((char*)&colorstr, "%02x.%02x.%02x", *r, *g, *b);
	frameBuffer->paintBoxRel(x+218,y+107, 80, 20, COL_MENUCONTENT);
	fonts->epg_date->RenderString(x+218,y+120, 80, colorstr, COL_MENUCONTENT);
	*/
}

int CColorChooser::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	setColor();
	paint();
	setColor();

	bool loop = true;

	int selected = 0;
	while(loop)
	{
		int key = g_RCInput->getKey(300);
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
				paintSlider(x+10, y+ hheight, r, "red", false);
				paintSlider(x+10, y+ hheight+ mheight, g, "green", false);
				paintSlider(x+ 10, y+ hheight+ mheight* 2, b, "blue", false);
				paintSlider(x+ 10, y+ hheight+ mheight* 3, alpha,"alpha",false);
				selected++;
				switch (selected)
				{
					case 0:
						paintSlider(x+ 10, y+ hheight, r, "red", true);
						break;
					case 1:
						paintSlider(x+ 10, y+ hheight+ mheight, g, "green", true);
						break;
					case 2:
						paintSlider(x+ 10, y+ hheight+ mheight* 2, b, "blue", true);
						break;
					case 3:
						paintSlider(x+ 10, y+ hheight+ mheight* 3, alpha, "alpha", true);
						break;
				}
			}
		}
		else if (key==CRCInput::RC_up)
		{
			if(selected>0)
			{
				paintSlider(x+10, y+hheight, r,"red", false);
				paintSlider(x+10, y+hheight+mheight, g,"green", false);
				paintSlider(x+10, y+hheight+mheight*2, b,"blue", false);
				paintSlider(x+10, y+hheight+mheight*3, alpha,"alpha",false);
				selected--;
				switch (selected)
				{
					case 0:
						paintSlider(x+10, y+hheight, r,"red", true);
						break;
					case 1:
						paintSlider(x+10, y+hheight+mheight, g,"green", true);
						break;
					case 2:
						paintSlider(x+10, y+hheight+mheight*2, b,"blue", true);
						break;
					case 3:
						paintSlider(x+10, y+hheight+mheight*3, alpha,"alpha",true);
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
						paintSlider(x+10, y+hheight, r,"red", true);
						setColor();
					}
					break;
				case 1:
					if (*g<100)
					{
						*g+=5;
						paintSlider(x+10, y+hheight+mheight, g,"green", true);
						setColor();
					}
					break;
				case 2:
					if (*b<100)
					{
						*b+=5;
						paintSlider(x+10, y+hheight+mheight*2, b,"blue", true);
						setColor();
					}
					break;
				case 3:
					if (*alpha<100)
					{
						*alpha+=5;
						paintSlider(x+10, y+hheight+mheight*3, alpha,"alpha",true);
						setColor();
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
						paintSlider(x+10, y+hheight, r,"red", true);
						setColor();
					}
					break;
				case 1:
					if (*g>0)
					{
						*g-=5;
						paintSlider(x+10, y+hheight+mheight, g,"green", true);
						setColor();
					}
					break;
				case 2:
					if (*b>0)
					{
						*b-=5;
						paintSlider(x+10, y+hheight+mheight*2, b,"blue", true);
						setColor();
					}
					break;
				case 3:
					if (*alpha>0)
					{
						*alpha-=5;
						paintSlider(x+10, y+hheight+mheight*3, alpha,"alpha",true);
						setColor();
					}
					break;
			}
		}
	}
	hide();
	if(observer)
		observer->changeNotify(name);
	return CMenuTarget::RETURN_REPAINT;
}

void CColorChooser::hide()
{
	g_FrameBuffer->paintBoxRel(x,y, width,height, COL_BACKGROUND);
}

void CColorChooser::paint()
{
	g_FrameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+hheight, width, name.c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT);

	paintSlider(x+10, y+hheight, r,"red", true);
	paintSlider(x+10, y+hheight+mheight, g,"green", false);
	paintSlider(x+10, y+hheight+mheight*2, b,"blue", false);
	paintSlider(x+10, y+hheight+mheight*3, alpha,"alpha",false);

	//color preview
	g_FrameBuffer->paintBoxRel(x+220,y+hheight, mheight*4,mheight*4, COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x+221,y+hheight+1, mheight*4-2,mheight*4-2, 254);
}

void CColorChooser::paintSlider(int x, int y, unsigned char *spos, string text, bool selected)
{
	if (!spos)
		return;
	g_FrameBuffer->paintBoxRel(x+70,y,120,mheight, COL_MENUCONTENT);
	g_FrameBuffer->paintIcon("volumebody.raw",x+70,y+2+mheight/4);
	string iconfile = "volumeslider2";
	if (selected)
		iconfile += text;
	iconfile +=".raw";
	g_FrameBuffer->paintIcon(iconfile,x+73+(*spos),y+mheight/4);

	g_Fonts->menu->RenderString(x,y+mheight, width, text.c_str(), COL_MENUCONTENT);
}

