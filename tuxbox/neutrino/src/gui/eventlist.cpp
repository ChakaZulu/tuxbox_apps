//
// $Id: eventlist.cpp,v 1.3 2001/09/18 11:34:42 fnbrd Exp $
//
// $Log: eventlist.cpp,v $
// Revision 1.3  2001/09/18 11:34:42  fnbrd
// Some changes.
//
// Revision 1.2  2001/09/18 11:05:58  field
// Ausgabe quick'n'dirty gefixt
//
// Revision 1.1  2001/09/18 10:50:30  fnbrd
// Eventlist, quick'n dirty
//
//

#include "eventlist.hpp"
#include "../include/debug.h"
#include "../global.h"

static char* copyStringto(const char* from, char* to, int len, char delim)
{
	const char *fromend=from+len;
	while(*from!=delim && from<fromend && *from)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

// quick'n dirty
void EventList::readEvents(const std::string& channelname)
{
  char rip[]="127.0.0.1";

  int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SAI servaddr;
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(sectionsd::portNumber);
  inet_pton(AF_INET, rip, &servaddr.sin_addr);

  if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1) {
    perror("Couldn't connect to sectionsd!");
    return;
  }
  sectionsd::msgRequestHeader req;
  req.version = 2;
  req.command = sectionsd::allEventsChannelName;
  req.dataLength = strlen(channelname.c_str())+1;
//  req.dataLength = 0;
  write(sock_fd, &req, sizeof(req));
  write(sock_fd, channelname.c_str(), req.dataLength);

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
  removeAllEvents(); // Alle gespeicherten Events loeschen
  char epgID[20];
  char edate[6];
  char etime[6];
  char eduration[10];
  char ename[100];
  char *actPos=pData;
  while(*actPos && actPos<pData+resp.dataLength) {
    *epgID=0;
    actPos = copyStringto( actPos, epgID, sizeof(epgID), ' ');
    printf("id: %s\n", epgID);
    *edate=0;
    actPos = copyStringto( actPos, edate, sizeof(edate), ' ');
    printf("date: %s\n", edate);
    *etime=0;
    actPos = copyStringto( actPos, etime, sizeof(etime), ' ');
    printf("time: %s\n", etime);
    *eduration=0;
    actPos = copyStringto( actPos, eduration, sizeof(eduration), ' ');
    printf("duration: %s\n", eduration);
    *ename=0;
    actPos = copyStringto( actPos, ename, sizeof(ename), '\n');
//    printf("desc: %s\n", channelDescription);
    event* evt = new event();
    evt->name=std::string(ename);
    evt->datetimeduration=std::string(edate);
    evt->datetimeduration+=std::string(" ");
    evt->datetimeduration+=std::string(etime);
    evt->datetimeduration+=std::string(" ");
    evt->datetimeduration+=std::string(eduration);
    printf("name: %s\n", evt->name.c_str());
//    tmp->number=number;
//    tmp->name=name;
    evtlist.insert(evtlist.end(), evt);
  }
  delete[] pData;
  return;
}

EventList::EventList(int Key=-1, const std::string &Name)
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

void EventList::removeAllEvents(void)
{
  for(unsigned int count=0; count<evtlist.size(); count++)
    delete evtlist[count];
  evtlist.clear();
}

EventList::~EventList()
{
  removeAllEvents();
}

void EventList::setName(const std::string& Name)
{
	name = Name;
}

void EventList::exec(const std::string& channelname)
{
  paintHead();
  readEvents(channelname);
  paint();
	
	int oldselected = selected;
	int zapOnExit = false;
	bool loop=true;
	while (loop)
	{
		int key = g_RCInput->getKey(100);
		if ((key==CRCInput::RC_timeout) || (key==g_settings.key_channelList_cancel))
		{
			selected = oldselected;
			loop=false;
		}
		else if (key==g_settings.key_channelList_pageup)
		{
			selected+=listmaxshow;
			if (selected>evtlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (key==g_settings.key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=evtlist.size()-1;
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
				selected = evtlist.size()-1;
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
			selected = (selected+1)%evtlist.size();
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
		}
	}
	hide();
/*
	if(zapOnExit)
	{
		zapTo(selected);
	}
*/
}

void EventList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void EventList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width, fheight, color);
	if(liststart+pos<evtlist.size())
	{
		event* evt = evtlist[liststart+pos];
		//number
//                char tmp[10];
//                sprintf((char*) tmp, "%d", chan->number);
//		int numpos = x+5+numwidth- g_Fonts->channellist_number->getRenderWidth(evt->name.c_str());
//		g_Fonts->channellist_number->RenderString(numpos,ypos+fheight, numwidth+5, evt->name.c_str(), color, fheight);
		printf("Rendering '%s'\n", evt->name.c_str());
		printf("date time duration '%s'\n", evt->datetimeduration.c_str());

//		if(strlen(chan->currentEvent.c_str()))
//		{
    			// name + description
//			char nameAndDescription[100];
//			snprintf(nameAndDescription, sizeof(nameAndDescription), "%s - %s", chan->name.c_str(), chan->currentEvent.c_str());
			g_Fonts->channellist->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width-numwidth-20, evt->name.c_str(), color);
/*                }
		else
		  //name
		  g_Fonts->channellist->RenderString(x+5+numwidth+10,ypos+fheight, width-numwidth-20, chan->name.c_str(), color);
*/
	}
}

void EventList::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(name).c_str(), COL_MENUHEAD);
}

void EventList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;
//	int lastnum =  evtlist[liststart]->number + listmaxshow;

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
}










