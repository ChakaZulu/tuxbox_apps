#include "bouqueteditor_bouquets.h"
#include "../global.h"

CBEBouquetWidget::CBEBouquetWidget()
{
	selected = 0;
	width = 500;
	height = 440;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
}

void CBEBouquetWidget::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);
	if(liststart+pos<Bouquets.size())
	{
		g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, Bouquets[liststart+pos].name, color);
	}
}

void CBEBouquetWidget::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;

	if(lastnum<10)
		numwidth = g_Fonts->channellist_number->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Fonts->channellist_number->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Fonts->channellist_number->getRenderWidth("00000");

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	g_FrameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((Bouquets.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}

void CBEBouquetWidget::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, "Bouquets" /*g_Locale->getText(name).c_str()*/, COL_MENUHEAD);
}

void CBEBouquetWidget::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

int CBEBouquetWidget::exec(CMenuTarget* parent, string actionKey)
{
	if (parent)
	{
		parent->hide();
	}

// getting all bouquets from zapit
	printf("getting bouquets\n");
	g_Zapit->getBouquets(Bouquets, true);
	printf("got bouquets\n");
	paintHead();
	paint();

	int oldselected = selected;
	bool loop=true;
	while (loop)
	{
		int key = g_RCInput->getKey(g_settings.timing_chanlist);
		if ((key==CRCInput::RC_timeout) || (key==g_settings.key_channelList_cancel))
		{
			selected = oldselected;
			loop=false;
		}
		else if (key==g_settings.key_channelList_pageup)
		{
			selected+=listmaxshow;
			if (selected>Bouquets.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (key==g_settings.key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=Bouquets.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (key==CRCInput::RC_up)
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = Bouquets.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if (key==CRCInput::RC_down)
		{
			int prevselected=selected;
			selected = (selected+1)%Bouquets.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if( (key==CRCInput::RC_spkr) || (key==CRCInput::RC_plus) || (key==CRCInput::RC_minus)
		         || (key==CRCInput::RC_red) || (key==CRCInput::RC_green) || (key==CRCInput::RC_yellow) || (key==CRCInput::RC_blue)
		         || (key==CRCInput::RC_standby)
		         || (CRCInput::isNumeric(key)) )
		{
			selected = oldselected;
			g_RCInput->pushbackKey (key);
			loop=false;
		}
	}
	hide();
	return RETURN_REPAINT;
}

