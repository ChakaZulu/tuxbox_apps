#include "stringinput.h"


CStringInput::CStringInput(string Name, FontsDef *Fonts, char* Value, int Size)
{
	name = Name;
	value = Value;
	fonts = Fonts;
	size = Size;
	width = 400;
	hheight = fonts->menu_title
->getHeight();
	mheight = fonts->menu
->getHeight();
	height = hheight+mheight+50;
	x=((720-width) >> 1) -50;
	y=((500-height)>>1);
	selected = 0;
}


int CStringInput::exec(CFrameBuffer* frameBuffer, CRCInput *rcInput,CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}

	for(int count=strlen(value)-1;count<size-1;count++)
		strcat(value, " ");
	
	paint( frameBuffer);

	bool loop = true;
	char validchars[] = "0123456789. ";

	//int selected = 0;
	while(loop)
	{
		int key = rcInput->getKey(300); 
		if(key==CRCInput::RC_timeout)
		{//timeout, close 
			loop = false;
		}
		else if (key==CRCInput::RC_left)
		{
			if(selected>0)
			{
				selected--;
				paintChar(frameBuffer, selected+1);
				paintChar(frameBuffer, selected);
			}
		}
		else if (key==CRCInput::RC_right)
		{
			if(selected<(int)strlen(value)-1)
			{
				selected++;
				paintChar(frameBuffer, selected-1);
				paintChar(frameBuffer, selected);
			}
		}
		else if (key==CRCInput::RC_up)
		{
			int npos = 0;
			for(int count=0;count<(int)strlen(validchars);count++)
				if(value[selected]==validchars[count])
					npos = count;
			npos++;
			if(npos>=(int)strlen(validchars))
				npos = 0;
			value[selected]=validchars[npos];
			paintChar(frameBuffer, selected);
		}
		else if (key==CRCInput::RC_down)
		{
			int npos = 0;
			for(int count=0;count<(int)strlen(validchars);count++)
				if(value[selected]==validchars[count])
					npos = count;
			npos--;
			if(npos<0)
				npos = strlen(validchars)-1;
			value[selected]=validchars[npos];
			paintChar(frameBuffer, selected);
		}
		else if (key==CRCInput::RC_ok)
		{
			loop=false;
		}

	}
	
	hide(frameBuffer);

	for(int count=size-1;count>=0;count--)
	{
		if((value[count]==' ') || (value[count]==0))
		{
			value[count]=0;
		}
		else
			break;
	}
	value[size]=0;
	return CMenuTarget::RETURN_REPAINT;
}

void CStringInput::hide(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,height, COL_BACKGROUND);
}

void CStringInput::paint(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+hheight, width, name.c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT);

	for (int count=0;count<size;count++)
		paintChar(frameBuffer, count);

}

void CStringInput::paintChar(CFrameBuffer* frameBuffer, int pos)
{
	int xs = 20;
	int ys = mheight;
	int xpos = x+20+ pos*xs;
	int ypos = y+hheight+25;

	string ch = "";
	if(pos<(int)strlen(value))
		ch = *(value+pos);

	int color = COL_MENUCONTENT;
	if (pos==selected)
		color = COL_MENUCONTENTSELECTED;

	frameBuffer->paintBoxRel(xpos,ypos, xs, ys, COL_MENUCONTENT+4);
	frameBuffer->paintBoxRel(xpos+1,ypos+1, xs-2, ys-2, color);

	int xfpos = xpos + ((xs-fonts->menu->getRenderWidth(ch.c_str()))>>1);

	fonts->menu->RenderString(xfpos,ypos+ys, width, ch.c_str(), color);
}


