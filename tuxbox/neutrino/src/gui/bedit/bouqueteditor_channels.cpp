#include "bouqueteditor_channels.h"
#include "bouqueteditor_chanselect.h"
#include "../global.h"

CBEChannelWidget::CBEChannelWidget(string Caption, unsigned int Bouquet)
{
	selected = 0;
	width = 500;
	height = 440;
	ButtonHeight = 25;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
	state = beDefault;
	caption = Caption;
	bouquet = Bouquet;
	mode = CZapitClient::MODE_TV;
}

void CBEChannelWidget::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);
	if ((liststart+pos==selected) && (state == beMoving))
	{
		g_FrameBuffer->paintIcon("gelb.raw", x + 8, ypos+4);
	}
	if(liststart+pos < Channels.size())
	{
		g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, Channels[liststart+pos].name, color);
	}
}

void CBEChannelWidget::paint()
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

	int sbc= ((Channels.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}

void CBEChannelWidget::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, caption.c_str() , COL_MENUHEAD);
}

void CBEChannelWidget::paintFoot()
{
	int ButtonWidth = width / 4;
	g_FrameBuffer->paintBoxRel(x,y+height, width,ButtonHeight, COL_MENUHEAD);
	g_FrameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW);

	g_FrameBuffer->paintIcon("rot.raw", x+width- 4* ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 4* ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.delete").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("gruen.raw", x+width- 3* ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.add").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("gelb.raw", x+width- 2* ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 2* ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.move").c_str(), COL_INFOBAR);

	g_FrameBuffer->paintIcon("blau.raw", x+width- ButtonWidth+ 8, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- ButtonWidth+ 29, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("bouqueteditor.switchmode").c_str(), COL_INFOBAR);
}

void CBEChannelWidget::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height+ButtonHeight);
}

int CBEChannelWidget::exec(CMenuTarget* parent, string actionKey)
{
	if (parent)
	{
		parent->hide();
	}

	Channels.clear();
	g_Zapit->getBouquetChannels( bouquet, Channels, mode);
	paintHead();
	paint();
	paintFoot();

	channelsChanged = false;
	int oldselected = selected;
	bool loop=true;
	while (loop)
	{
		int key = g_RCInput->getKey();
		if ((key==CRCInput::RC_timeout) || (key==g_settings.key_channelList_cancel))
		{
			if (state == beDefault)
			{
				loop = false;
			}
			else if (state == beMoving)
			{
				cancelMoveChannel();
			}
		}
		else if (key==CRCInput::RC_up)
		{
			if (state == beDefault)
			{
				int prevselected=selected;
				if(selected==0)
				{
					selected = Channels.size()-1;
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
			else if (state == beMoving)
			{
				internalMoveChannel(selected, selected - 1);
			}
		}
		else if (key==CRCInput::RC_down)
		{
			if (state == beDefault)
			{
				int prevselected=selected;
				selected = (selected+1)%Channels.size();
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
			else if (state == beMoving)
			{
				internalMoveChannel(selected, selected + 1);
			}
		}
		else if(key==CRCInput::RC_red)
		{
			if (state == beDefault)
				deleteChannel();
		}
		else if(key==CRCInput::RC_green)
		{
			if (state == beDefault)
				addChannel();
		}
		else if(key==CRCInput::RC_blue)
		{
			if (state == beDefault)
			{
				if (mode == CZapitClient::MODE_TV)
					mode = CZapitClient::MODE_RADIO;
				else
					mode = CZapitClient::MODE_TV;
				Channels.clear();
				g_Zapit->getBouquetChannels( bouquet, Channels, mode);
				paint();
			}
		}
		else if(key==CRCInput::RC_yellow)
		{
			liststart = (selected/listmaxshow)*listmaxshow;
			if (state == beDefault)
				beginMoveChannel();
			paintItem(selected - liststart);
		}
		else if(key==CRCInput::RC_ok)
		{
			if (state == beDefault)
			{
			}
			else if (state == beMoving)
			{
				finishMoveChannel();
			}
		}
		else if( (key==CRCInput::RC_spkr) || (key==CRCInput::RC_plus) || (key==CRCInput::RC_minus)
		         || (key==CRCInput::RC_standby)
		         || (CRCInput::isNumeric(key)) )
		{
			if (state == beDefault)
			{
				//kein pushback - wenn man versehentlich wo draufkommt is die edit-arbeit umsonst
				//selected = oldselected;
				//g_RCInput->pushbackKey (key);
				//loop=false;
			}
			else if (state == beMoving)
			{
				cancelMoveChannel();
			}
		}
	}
	hide();
	return RETURN_REPAINT;
}

void CBEChannelWidget::deleteChannel()
{
	g_Zapit->removeChannelFromBouquet( bouquet, Channels[selected].onid_sid);
	Channels.clear();
	g_Zapit->getBouquetChannels( bouquet, Channels, mode);
	if (selected >= Channels.size())
		selected--;
	channelsChanged = true;
	paint();
}

void CBEChannelWidget::addChannel()
{
	printf("new\n");
	CBEChannelSelectWidget* channelSelectWidget = new CBEChannelSelectWidget(caption, bouquet, mode);
	printf("exec\n");
	channelSelectWidget->exec(this, "");
	printf("exec done\n");
	if (channelSelectWidget->hasChanged())
	{
		channelsChanged = true;
		Channels.clear();
		g_Zapit->getBouquetChannels( bouquet, Channels, mode);
	}
	delete channelSelectWidget;
	paintHead();
	paint();
	paintFoot();
}

void CBEChannelWidget::beginMoveChannel()
{
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEChannelWidget::finishMoveChannel()
{
	state = beDefault;
	if (newPosition != origPosition)
	{
		g_Zapit->moveChannel( bouquet, origPosition+1, newPosition+1, mode);
		Channels.clear();
		g_Zapit->getBouquetChannels( bouquet, Channels, mode);
		channelsChanged = true;
	}
	paint();
}

void CBEChannelWidget::cancelMoveChannel()
{
	state = beDefault;
	internalMoveChannel( newPosition, origPosition);
}

void CBEChannelWidget::internalMoveChannel( unsigned int fromPosition, unsigned int toPosition)
{
	if ( toPosition == -1 ) return;
	if ( toPosition == Channels.size()) return;

	CZapitClient::responseGetBouquetChannels Channel = Channels[fromPosition];
	if (fromPosition < toPosition)
	{
		for (unsigned int i=fromPosition; i<toPosition; i++)
			Channels[i] = Channels[i+1];
	}
	else if (fromPosition > toPosition)
	{
		for (unsigned int i=fromPosition; i>toPosition; i--)
			Channels[i] = Channels[i-1];
	}
	Channels[toPosition] = Channel;
	selected = toPosition;
	newPosition = toPosition;
	paint();
}

bool CBEChannelWidget::hasChanged()
{
	return (channelsChanged);
}

