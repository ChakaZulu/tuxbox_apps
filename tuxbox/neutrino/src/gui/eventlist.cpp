//
// $Id: eventlist.cpp,v 1.8 2001/09/20 00:36:32 field Exp $
//
// $Log: eventlist.cpp,v $
// Revision 1.8  2001/09/20 00:36:32  field
// epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
// Revision 1.7  2001/09/19 00:11:57  McClean
// dont add events with "" Text
//
// Revision 1.6  2001/09/18 20:20:26  field
// Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
// vorbereitet
//
// Revision 1.5  2001/09/18 15:26:09  field
// Handelt nun auch "kein epg"
//
// Revision 1.4  2001/09/18 14:58:20  field
// Eventlist verbessert
//
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

    removeAllEvents(); // Alle gespeicherten Events loeschen
    current_event = -1;

    if ( resp.dataLength>0 )
    {
        char* pData = new char[resp.dataLength] ;
        if(read(sock_fd, pData, resp.dataLength)<=0)
        {
            delete[] pData;
            close(sock_fd);
            printf("EventList::readEvents - read from socket failed!");
            return;
        }
        char epgID[20];
        char edate[6];
        char etime[6];
        char eduration[10];
        char ename[100];
        char *actPos=pData;

        struct  tm   *tmZeit;
        int     mm, dd, hh, mn, evtTime, aktTime;
        time_t  tZeit  = time(NULL);
        tmZeit = localtime(&tZeit);
        aktTime = (tmZeit->tm_mon+ 1)* 1000000+ (tmZeit->tm_mday)* 10000+ (tmZeit->tm_hour)* 100+ tmZeit->tm_min;

        while(*actPos && actPos<pData+resp.dataLength)
        {
            *epgID=0;
            actPos = copyStringto( actPos, epgID, sizeof(epgID), ' ');
//    printf("id: %s\n", epgID);
            *edate=0;
            actPos = copyStringto( actPos, edate, sizeof(edate), ' ');
//    printf("date: %s\n", edate);
            *etime=0;
            actPos = copyStringto( actPos, etime, sizeof(etime), ' ');
//    printf("time: %s\n", etime);
            *eduration=0;
            actPos = copyStringto( actPos, eduration, sizeof(eduration), ' ');
//    printf("duration: %s\n", eduration);
            *ename=0;
            actPos = copyStringto( actPos, ename, sizeof(ename), '\n');
//    printf("desc: %s\n", channelDescription);

            event* evt = new event();

            sscanf(edate, "%02d.%02d", &dd, &mm);
            sscanf(etime, "%02d:%02d", &hh, &mn);
            evtTime = mm* 1000000+ dd* 10000+ hh* 100+ mn;

            if ( (evtTime- aktTime) < 0 )
                current_event++;

            evt->name=std::string(ename);
            evt->datetimeduration=std::string(edate);
            evt->datetimeduration+=std::string(" ");
            evt->datetimeduration+=std::string(etime);
            evt->datetimeduration+=std::string(" (");
            evt->datetimeduration+=std::string(eduration);
            evt->datetimeduration+=std::string(" m)");

//            printf("id: %s - name: %s\n", epgID, evt->name.c_str());
//    tmp->number=number;
//    tmp->name=name;
            if(evt->name !="")
                evtlist.insert(evtlist.end(), evt);
        }

        delete[] pData;
    }
    close(sock_fd);

    if ( current_event == -1 )
    {
        event* evt = new event();

        evt->name= g_Locale->getText("epglist.noevents") ;
        evt->datetimeduration= std::string("");
        evtlist.insert(evtlist.end(), evt);
        current_event++;
    }
    selected= current_event;

    return;
}

EventList::EventList()
{
	selected = 0;
    current_event = 0;

	width = 580;
	height = 440;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
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

void EventList::exec(const std::string& channelname)
{
  name = channelname;
  paintHead();
  readEvents(channelname);
  paint();
	
	int oldselected = selected;
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
			loop=false;
		}
	}
	hide();
}

void EventList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void EventList::paintItem(int pos)
{
    int color;
	int ypos = y+ theight+0 + pos*fheight;

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}
    else if (liststart+pos == current_event )
	{
		color = COL_MENUCONTENTINACTIVE;
	}
    else
   	{
		color = COL_MENUCONTENT;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width, fheight, color);

	if(liststart+pos<evtlist.size())
	{
		event* evt = evtlist[liststart+pos];

//		printf("Rendering '%s'\n", evt->name.c_str());
//		printf("date time duration '%s'\n", evt->datetimeduration.c_str());


        g_Fonts->channellist_number->RenderString(x+ 10, ypos+ fheight, 157, evt->datetimeduration.c_str(), color, fheight);
		g_Fonts->channellist->RenderString(x+ 170, ypos+ fheight, width- 180, evt->name.c_str(), color);
	}
}

void EventList::paintHead()
{
    char l_name[100];
    snprintf(l_name, sizeof(l_name), g_Locale->getText("epglist.head").c_str(), name.c_str() );

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, l_name, COL_MENUHEAD);
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


