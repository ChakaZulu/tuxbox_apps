#include "stringinput.h"
#include "../global.h"

CStringInput::CStringInput(string Name, char* Value, int Size,  string Hint_1, string Hint_2, char* Valid_Chars, CChangeObserver* Observ)
{
	name = Name;
	value = Value;
	size = Size;

    hint_1 = Hint_1;
    hint_2 = Hint_2;
    validchars = Valid_Chars;

    observ = Observ;

	width = 420;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
    iheight = g_Fonts->menu_info->getHeight();

    height = hheight+ mheight+ 50;
    if ( hint_1.length()> 0 )
        height+= iheight;
    if ( hint_2.length()> 0 )
        height+= iheight;

	x = ((720-width) >> 1) -50;
	y = ((500-height)>>1);
	selected = 0;
}


int CStringInput::exec( CMenuTarget* parent, string )
{
    char oldval[size];
    int key;

    strcpy(oldval, value);

	if (parent)
	{
		parent->hide();
	}

	for(int count=strlen(value)-1;count<size-1;count++)
		strcat(value, " ");
	
	paint();

	bool loop = true;

	//int selected = 0;
	while(loop)
	{
		key = g_RCInput->getKey(300);
		if (key==CRCInput::RC_left)
		{
			if(selected>0)
			{
				selected--;
				paintChar(selected+1);
				paintChar(selected);
			}
		}
		else if (key==CRCInput::RC_right)
		{
			if(selected<(int)strlen(value)-1)
			{
				selected++;
				paintChar(selected-1);
				paintChar(selected);
			}
		}
        else if ( ( key>= 0 ) && ( key<= 9) )
		{
            value[selected]=validchars[key];
			paintChar(selected);

            if (selected < (size- 1))
                selected++;
            paintChar(selected-1);
			paintChar(selected);
		}
        else if ( (key==CRCInput::RC_red) && ( strstr(validchars, " ")!=NULL ) )
		{
            value[selected]=' ';
			paintChar(selected);

            if (selected < (size- 1))
                selected++;
            paintChar(selected-1);
			paintChar(selected);
		}
        else if ( (key==CRCInput::RC_green) && ( strstr(validchars, ".")!=NULL ) )
		{
            value[selected]='.';
			paintChar(selected);

            if (selected < (size- 1))
                selected++;
            paintChar(selected-1);
			paintChar(selected);
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
			paintChar(selected);
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
			paintChar(selected);
		}
		else if (key==CRCInput::RC_ok)
		{
			loop=false;
		}
        else if ( (key==CRCInput::RC_home) || (key==CRCInput::RC_timeout) )
		{
            strcpy(value, oldval);
			loop=false;
        }

	}
	
	hide();

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

    if ( (observ) && (key==CRCInput::RC_ok) )
	{
		observ->changeNotify( value );
	}

	return CMenuTarget::RETURN_REPAINT;
}

void CStringInput::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CStringInput::paint()
{
	g_FrameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width, g_Locale->getText(name).c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, y+ hheight, width, height- hheight, COL_MENUCONTENT);

    if ( hint_1.length()> 0 )
    {
        g_Fonts->menu_info->RenderString(x+ 20, y+ hheight+ mheight+ iheight+ 40, width- 20, g_Locale->getText(hint_1).c_str(), COL_MENUCONTENT);
        if ( hint_2.length()> 0 )
            g_Fonts->menu_info->RenderString(x+ 20, y+ hheight+ mheight+ iheight* 2+ 40, width- 20, g_Locale->getText(hint_2).c_str(), COL_MENUCONTENT);
    }

	for (int count=0;count<size;count++)
		paintChar(count);

}

void CStringInput::paintChar(int pos)
{
	int xs = 20;
	int ys = mheight;
	int xpos = x+ 20+ pos* xs;
	int ypos = y+ hheight+ 25;

	string ch = "";
	if(pos<(int)strlen(value))
		ch = *(value+pos);

	int color = COL_MENUCONTENT;
	if (pos==selected)
		color = COL_MENUCONTENTSELECTED;

	g_FrameBuffer->paintBoxRel(xpos, ypos, xs, ys, COL_MENUCONTENT+ 4);
	g_FrameBuffer->paintBoxRel(xpos+ 1, ypos+ 1, xs- 2, ys- 2, color);

	int xfpos = xpos + ((xs- g_Fonts->menu->getRenderWidth(ch.c_str()))>>1);

	g_Fonts->menu->RenderString(xfpos,ypos+ys, width, ch.c_str(), color);
}


