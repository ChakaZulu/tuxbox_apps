#include "colorchooser.h"


CColorChooser::CColorChooser(string Name, FontsDef *Fonts, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer)
{
	observer = Observer;
	name = Name;
	fonts = Fonts;
	width = 360;
	hheight = fonts->menu_title->getHeight();
	mheight = fonts->menu->getHeight();
	height = hheight+mheight*4;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	r = R;
	g = G;
	b = B;
	alpha = Alpha;
}

void CColorChooser::setColor(CFrameBuffer* frameBuffer)
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

int CColorChooser::exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}
	setColor(frameBuffer);
	paint( frameBuffer);
	setColor(frameBuffer);

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
				paintSlider(frameBuffer, x+10, y+hheight, r,"red", false);
				paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", false);
				paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", false);
				paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",false);
				selected++;
				switch (selected)
				{
					case 0:
						paintSlider(frameBuffer, x+10, y+hheight, r,"red", true);
						break;
					case 1:
						paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", true);
						break;
					case 2:
						paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", true);
						break;
					case 3:
						paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",true);
						break;
				}
			}
		}
		else if (key==CRCInput::RC_up)
		{
			if(selected>0)
			{
				paintSlider(frameBuffer, x+10, y+hheight, r,"red", false);
				paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", false);
				paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", false);
				paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",false);
				selected--;
				switch (selected)
				{
					case 0:
						paintSlider(frameBuffer, x+10, y+hheight, r,"red", true);
						break;
					case 1:
						paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", true);
						break;
					case 2:
						paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", true);
						break;
					case 3:
						paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",true);
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
						paintSlider(frameBuffer, x+10, y+hheight, r,"red", true);
						setColor(frameBuffer);
					}
					break;
				case 1:
					if (*g<100)
					{
						*g+=5;
						paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", true);
						setColor(frameBuffer);
					}
					break;
				case 2:
					if (*b<100)
					{
						*b+=5;
						paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", true);
						setColor(frameBuffer);
					}
					break;
				case 3:
					if (*alpha<100)
					{
						*alpha+=5;
						paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",true);
						setColor(frameBuffer);
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
						paintSlider(frameBuffer, x+10, y+hheight, r,"red", true);
						setColor(frameBuffer);
					}
					break;
				case 1:
					if (*g>0)
					{
						*g-=5;
						paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", true);
						setColor(frameBuffer);
					}
					break;
				case 2:
					if (*b>0)
					{
						*b-=5;
						paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", true);
						setColor(frameBuffer);
					}
					break;
				case 3:
					if (*alpha>0)
					{
						*alpha-=5;
						paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",true);
						setColor(frameBuffer);
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

void CColorChooser::paint(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+hheight, width, name.c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT);

	paintSlider(frameBuffer, x+10, y+hheight, r,"red", true);
	paintSlider(frameBuffer, x+10, y+hheight+mheight, g,"green", false);
	paintSlider(frameBuffer, x+10, y+hheight+mheight*2, b,"blue", false);
	paintSlider(frameBuffer, x+10, y+hheight+mheight*3, alpha,"alpha",false);

	//color preview
	frameBuffer->paintBoxRel(x+220,y+hheight, mheight*4,mheight*4, COL_MENUHEAD);
	frameBuffer->paintBoxRel(x+221,y+hheight+1, mheight*4-2,mheight*4-2, 254);
}

void CColorChooser::paintSlider(CFrameBuffer* frameBuffer, int x, int y, unsigned char *spos, string text, bool selected)
{
	if (!spos)
		return;
	frameBuffer->paintBoxRel(x+70,y,120,mheight, COL_MENUCONTENT);
	frameBuffer->paintIcon("volumebody.raw",x+70,y+2+mheight/4);
	string iconfile = "volumeslider2";
	if (selected)
		iconfile += text;
	iconfile +=".raw";
	frameBuffer->paintIcon(iconfile,x+73+(*spos),y+mheight/4);

	fonts->menu->RenderString(x,y+mheight, width, text.c_str(), COL_MENUCONTENT);
}
