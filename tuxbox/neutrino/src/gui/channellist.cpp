//
// $Id: channellist.cpp,v 1.13 2001/09/06 12:04:10 McClean Exp $
//
// $Log: channellist.cpp,v $
// Revision 1.13  2001/09/06 12:04:10  McClean
// fix neutrino-crash (no chanlist)
//
// Revision 1.12  2001/09/03 03:34:04  tw-74
// cosmetic fixes, own "Mg" fontmetrics
//
// Revision 1.11  2001/08/21 00:30:38  tw-74
// more fontrendering (see comments there), screen cosmetics
//
// Revision 1.10  2001/08/20 13:10:27  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.9  2001/08/20 01:51:12  McClean
// channellist bug fixed - faster channellist response
//
// Revision 1.8  2001/08/20 01:26:54  McClean
// stream info added
//
// Revision 1.7  2001/08/16 23:24:17  McClean
// positioning and display-clear bug fixed
//
// Revision 1.6  2001/08/16 23:19:18  McClean
// epg-view and quickview changed
//
// Revision 1.5  2001/08/15 17:02:26  fnbrd
// Channellist now wider
//
//

#include "channellist.h"
#include "../include/debug.h"

static char* copyStringto(const char* from, char* to, int len)
{
	const char *fromend=from+len;
	while(*from!='\n' && from<fromend && *from)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

// quick'n dirty
void CChannelList::updateEvents(void)
{
	char rip[]="127.0.0.1";

	int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SAI servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(sectionsd::portNumber);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to sectionsd!");
		return;
	}
  sectionsd::msgRequestHeader req;
  req.version = 2;
  req.command = sectionsd::actualEventListTVshort;
  req.dataLength = 0;
  write(sock_fd,&req,sizeof(req));

  sectionsd::msgResponseHeader resp;
  memset(&resp, 0, sizeof(resp));
  if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0) {
    close(sock_fd);
    return;
  }
  if(resp.dataLength<=0) {
    close(sock_fd);
    return;
  }

  char* pData = new char[resp.dataLength] ;
  if(read(sock_fd, pData, resp.dataLength)<=0) {
    delete[] pData;
    close(sock_fd);
    return;
  }
  close(sock_fd);
  for(unsigned int count=0;count<chanlist.size();count++)
    chanlist[count]->currentEvent="";
  char epgID[20];
  char channelName[50];
  char channelDescription[1000];
  char *actPos=pData;
  while(*actPos && actPos<pData+resp.dataLength) {
    *epgID=0;
    actPos = copyStringto( actPos, epgID, sizeof(epgID));
//    printf("id: %s\n", epgID);
    *channelName=0;
    actPos = copyStringto( actPos, channelName, sizeof(channelName));
//    printf("name: %s\n", channelName);
    *channelDescription=0;
    actPos = copyStringto( actPos, channelDescription, sizeof(channelDescription));
//    printf("desc: %s\n", channelDescription);
    // quick'n dirty, sollte man mal anders machen
    for(unsigned int count=0;count<chanlist.size();count++) {
      if(!strcasecmp(chanlist[count]->name.c_str(), channelName)) {
        chanlist[count]->currentEvent=channelDescription;
//	printf("Channel found\n");
	break;
      }
    }
  }
  delete[] pData;
  return;
}

CChannelList::CChannelList(SNeutrinoSettings *settings, int Key=-1, string Name="", FontsDef *Fonts)
{
	fonts = Fonts;
	key = Key;
	name = Name;
	selected = 0;
	width = 500;
	height = 440;
	theight=fonts->menu_title->getHeight();
	fheight=fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((settings->screen_EndX-settings->screen_StartX)-width) / 2) + settings->screen_StartX;
	y=(((settings->screen_EndY-settings->screen_StartY)-height) / 2) + settings->screen_StartY;
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

int CChannelList::getActiveChannelNumber()
{
	return selected+1;
}


void CChannelList::exec(CFrameBuffer* frameBuffer, CRCInput* rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings)
{
	if(chanlist.size()==0)
	{
		//evtl. anzeige dass keine kanalliste....
		return;
	}
	paintHead(frameBuffer);
	updateEvents();
	paint(frameBuffer);
	
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
			paint(frameBuffer);
		}
		else if (key==settings->key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=chanlist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint(frameBuffer);
		}
		else if (key==CRCInput::RC_up)
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = chanlist.size()-1;
			}
			else selected--;
			paintItem(frameBuffer, prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint(frameBuffer);
			}
			else
			{
				paintItem(frameBuffer, selected - liststart);
			}
		}
		else if (key==CRCInput::RC_down)
		{
			int prevselected=selected;
			selected = (selected+1)%chanlist.size();
			paintItem(frameBuffer, prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint(frameBuffer);
			}
			else
			{
				paintItem(frameBuffer, selected - liststart);
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
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
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

void CChannelList::numericZap(CFrameBuffer *frameBuffer, CRCInput *rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, int key)
{
        if(chanlist.size()==0)
        {
                //evtl. anzeige dass keine kanalliste....
                return;
        }
 
	int ox=300, oy=200;
	int sx=fonts->channellist->getRenderWidth("000")+14, sy=fonts->channellist->getHeight()+6;
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
		fonts->channellist->RenderString(ox+7, oy+sy-3, sx, valstr, COL_INFOBAR);
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
	fonts->channellist->RenderString(ox+7, oy+sy-3, sx, valstr, COL_INFOBAR);
	usleep(100000);
	frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_BACKGROUND);
	chn--;
	if (chn<0)
		chn=0;
	zapTo( remoteControl, infoViewer, chn);
}

void CChannelList::quickZap(CFrameBuffer* frameBuffer, CRCInput* rcInput, CRemoteControl *remoteControl, CInfoViewer *infoViewer, SNeutrinoSettings* settings, int key)
{
        if(chanlist.size()==0)
        {
                //evtl. anzeige dass keine kanalliste....
                return;
        }
 
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

void CChannelList::paintItem(CFrameBuffer* frameBuffer, int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	frameBuffer->paintBoxRel(x,ypos, width, fheight, color);
	if(liststart+pos<chanlist.size())
	{
		channel* chan = chanlist[liststart+pos];
		//number
                char tmp[10];
                sprintf((char*) tmp, "%d", chan->number);
		int numpos = x+5+numwidth-fonts->channellist_number->getRenderWidth(tmp);
		fonts->channellist_number->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);
		if(strlen(chan->currentEvent.c_str())) {
    		  // name + description
		  char nameAndDescription[100];
		  snprintf(nameAndDescription, sizeof(nameAndDescription), "%s - %s", chan->name.c_str(), chan->currentEvent.c_str());
		  fonts->channellist->RenderString(x+5+numwidth+10,ypos+fheight, width-numwidth-20, nameAndDescription, color);
                }
		else
		  //name
		  fonts->channellist->RenderString(x+5+numwidth+10,ypos+fheight, width-numwidth-20, chan->name.c_str(), color);
	}
}

void CChannelList::paintHead(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+theight+0, width, name.c_str(), COL_MENUHEAD);
}

void CChannelList::paint(CFrameBuffer* frameBuffer)
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  chanlist[liststart]->number + listmaxshow;

	if(lastnum<10)
	    numwidth = fonts->channellist_number->getRenderWidth("0");
	else if(lastnum<100)
	    numwidth = fonts->channellist_number->getRenderWidth("00");
	else if(lastnum<1000)
	    numwidth = fonts->channellist_number->getRenderWidth("000");
	else if(lastnum<10000)
	    numwidth = fonts->channellist_number->getRenderWidth("0000");
	else // if(lastnum<100000)
	    numwidth = fonts->channellist_number->getRenderWidth("00000");
	
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(frameBuffer, count);
	}
}
