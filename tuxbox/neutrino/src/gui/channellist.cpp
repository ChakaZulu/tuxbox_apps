#include "channellist.h"


CChannelList::CChannelList(int Key=-1, string Name="")
{
	key = Key;
	name = Name;
	selected = 0;
	width = 300;
	height = 400;
	x=((720-width) >> 1) - 40;
	y=((576-height)>>1)-20;
	listmaxshow = 17;
	liststart = 0;
	tuned=0xfffffff;
}

CChannelList::~CChannelList()
{
	for(unsigned int count=0;count<chanlist.size();count++)
	{
		delete chanlist[count];
	}
	chanlist.clear();
}

void CChannelList::addChannel(int key, int number, string name)
{
	channel* tmp = new channel();
	tmp->key=key;
	tmp->number=number;
	tmp->name=name;
	chanlist.insert(chanlist.end(), tmp);

}

void CChannelList::setName(string Name)
{
	name = Name;
}

int CChannelList::getKey(int id)
{
	return chanlist[id]->key;
}

string CChannelList::getActiveChannelName()
{
	return chanlist[selected]->name;
}

void CChannelList::exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings)
{
	paint(frameBuffer, fonts);
	int oldselected = selected;
	int zapOnExit = false;
	bool loop=true;
	while (loop)
	{
		int key = rcInput->getKey(100); 
		if ((key==CRCInput::RC_timeout) || (key==settings->key_channelList_cancel))
		{
			selected = oldselected;
			loop=false;
		}
		else if (key==settings->key_channelList_pageup)
		{
			selected+=listmaxshow;
			if (selected>chanlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint(frameBuffer, fonts);
		}
		else if (key==settings->key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=chanlist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint(frameBuffer, fonts);
		}
		else if (key==CRCInput::RC_up)
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = chanlist.size()-1;
			}
			else selected--;
			paintItem(frameBuffer, fonts, prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint(frameBuffer, fonts);
			}
			else 
			{	
				paintItem(frameBuffer, fonts, selected - liststart);
			}
		}
		else if (key==CRCInput::RC_down)
		{
			int prevselected=selected;
			selected = (selected+1)%chanlist.size();
			paintItem(frameBuffer, fonts, prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint(frameBuffer, fonts);
			}
			else 
			{	
				paintItem(frameBuffer, fonts, selected - liststart);
			}
		}
		else if (key==CRCInput::RC_ok)
		{
			zapOnExit = true;
			loop=false;
		}
	}
	hide(frameBuffer);
	if(zapOnExit)
	{
		zapTo(remoteControl,infoViewer, selected);
	}
}

void CChannelList::hide(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,height, 255);
}

void CChannelList::paintItem(CFrameBuffer* frameBuffer, FontsDef *fonts, int pos)
{
	int ypos = y+ 35 + pos*20;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	frameBuffer->paintBoxRel(x,ypos, width,20, color);
	if(liststart+pos<chanlist.size())
	{
		channel* chan = chanlist[liststart+pos];
		//number
                char tmp[10];
                sprintf((char*) tmp, "%d", chan->number);
		int numpos = x+5+numwidth-fonts->menu_number->getRenderWidth(tmp);
		fonts->menu_number->RenderString(numpos,ypos+16, numwidth+5, tmp, color);
		//name
		fonts->menu->RenderString(x+5+numwidth+5,ypos+17, width-numwidth-20, chan->name.c_str(), color);
	}
}

bool CChannelList::showInfo(CInfoViewer *infoViewer, int pos)
{
	if((pos >= (signed int) chanlist.size()) || (pos<0))
	{
		return false;
	}
	selected=pos;
	channel* chan = chanlist[selected];
	infoViewer->showTitle(selected+1, chan->name);
	return true;
}

void CChannelList::zapTo(CRemoteControl *remoteControl, CInfoViewer *infoViewer, int pos)
{
	if((pos >= (signed int) chanlist.size()) || (pos<0) || (pos==(int)tuned))
	{
		return;
	}
	tuned = pos;
	showInfo(infoViewer, pos);
	channel* chan = chanlist[selected];
	remoteControl->zapTo(chan->key, chan->name);
}

void CChannelList::numericZap(CFrameBuffer *frameBuffer, FontsDef *fonts, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, int key)
{
  int ox=300, oy=200, sx=50, sy=22;
  char valstr[10];
  int chn=key;
  int pos=1;

  while(1)
  {
    sprintf((char*) &valstr, "%d",chn);
    while(strlen(valstr)<3)
    {
      strcat(valstr,"-");
    }
    frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR);
    fonts->channellist->RenderString(ox+7, oy+18, sx, valstr, COL_INFOBAR);
	if(!showInfo(infoViewer, chn-1))
	{	//channelnumber out of bounds
		infoViewer->killTitle(); //warum tut das net?
		usleep(100000);		
		frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_BACKGROUND);
		return;
	}

    if ((key=rcInput->getKey(30))==-1)
      break;

    if ((key>=0) && (key<=9))
    { //numeric
      chn=chn*10+key;
      pos++;
      if(pos==3)
      {
        break;
      }
    }
    else if (key==CRCInput::RC_ok)
    {
      break;
    }
  }
  //channel selected - show+go
  frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR);
  sprintf((char*) &valstr, "%d",chn);
  while(strlen(valstr)<3)
  {
    strcat(valstr,"-");
  }
  fonts->channellist->RenderString(ox+7, oy+18, sx, valstr, COL_INFOBAR);
  usleep(100000);
  frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_BACKGROUND);
  chn--;
  if (chn<0)
    chn=0;
  zapTo( remoteControl, infoViewer, chn);
}

void CChannelList::quickZap(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings, int key)
{
	printf("quickzap start\n");
	while(1)
	{
		if (key==settings->key_quickzap_down)
		{
			if(selected==0)
					selected = chanlist.size()-1;
				else
					selected--;
				channel* chan = chanlist[selected];
			infoViewer->showTitle( selected+1, chan->name);
		}
		else if (key==settings->key_quickzap_up)
		{
			selected = (selected+1)%chanlist.size();
			channel* chan = chanlist[selected];
			infoViewer->showTitle(selected+1, chan->name);
		}
		else
		{
			zapTo(remoteControl, infoViewer,  selected);
			break;
		}
		key = rcInput->getKey(7); 
	}
}

void CChannelList::paint(CFrameBuffer* frameBuffer, FontsDef *fonts)
{
	frameBuffer->paintBoxRel(x,y, width,30, COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+30, width,5, COL_MENUCONTENT);

	fonts->menu_title->RenderString(x+10,y+23, width, name.c_str(), COL_MENUHEAD);
	
	liststart = (selected/listmaxshow)*listmaxshow;


	int lastnum =  chanlist[liststart]->number + listmaxshow;
	string map = "";
	while(lastnum>0)
	{
		map += "0";
		lastnum = lastnum/10;
	}
	numwidth = fonts->menu_number->getRenderWidth(map.c_str());
	
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(frameBuffer, fonts, count );
	}

}
