#include "../include/debug.h"
#include "../global.h"

CBouquetList::CBouquetList(int Key=-1, const std::string &Name)
{
	key = Key;
	name = Name;
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
	tuned=0xfffffff;
}

CBouquetList::~CBouquetList()
{
	for(unsigned int count=0;count<Bouquets.size();count++)
	{
		delete Bouquets[count];
	}
	Bouquets.clear();
}

CBouquet* CBouquetList::addBouquet(const std::string& name)
{
	CBouquet* tmp = new CBouquet(Bouquets.size(), name);
	Bouquets.insert(Bouquets.end(), tmp);
	return(tmp);
}

void CBouquetList::setName(const std::string& Name)
{
	name = Name;
}

const std::string& CBouquetList::getActiveBouquetName()
{
	return Bouquets[selected]->name;
}

int CBouquetList::getActiveBouquetNumber()
{
	return selected;
}

bool CBouquetList::showChannelList( int nBouquet = -1)
{
	if (nBouquet == -1)
		nBouquet = selected;

	int nNewChannel = Bouquets[nBouquet]->channelList->show();
	if (nNewChannel > -1)
	{
		selected = nBouquet;
		orgChannelList->zapTo(Bouquets[selected]->channelList->getKey(nNewChannel)-1);
		return(true);
	}
	return(false);
}

void CBouquetList::adjustToChannel( int nChannelNr)
{
	for (uint i=0; i<Bouquets.size(); i++)
	{
		if (Bouquets[i]->channelList->hasChannel(nChannelNr))
		{
			selected = i;
			// TODO: adjust bouquet channel list to master channel list
			return;
		}
	}
}


void CBouquetList::activateBouquet( int id, bool bShowChannelList = false)
{
	selected = id;
	if (bShowChannelList)
	{
		int nNewChannel = Bouquets[selected]->channelList->show();

		if (nNewChannel > -1)
			orgChannelList->zapTo(Bouquets[selected]->channelList->getKey(nNewChannel)-1);
	}
}

void CBouquetList::exec( bool bShowChannelList)
{
	if (show() > -1)
	{
		activateBouquet( selected, bShowChannelList);
	}
}

int CBouquetList::show()
{
	if(Bouquets.size()==0)
	{
		return -1;
	}
	paintHead();
	paint();

	int oldselected = selected;
	int zapOnExit = false;
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
			else selected--;
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
		else if (key==CRCInput::RC_ok)
		{
			zapOnExit = true;
			loop=false;
		} else {
			selected = oldselected;
			g_RCInput->pushbackKey (key);
			loop=false;
		}
	}
	hide();
	if(zapOnExit)
	{
		return (selected);
	}
	else
	{
		return (-1);
	}
}

void CBouquetList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


void CBouquetList::paintItem(int pos)
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
        CBouquet* bouq = Bouquets[liststart+pos];
		//number

		g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, bouq->name.c_str(), color);
	}
}

void CBouquetList::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, "Bouquets" /*g_Locale->getText(name).c_str()*/, COL_MENUHEAD);
}

void CBouquetList::paint()
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

