//
// $Id: eventlist.cpp,v 1.18 2001/10/22 15:24:48 McClean Exp $
//
//  -- EPG Event List // Vorschau 
//
//
//
// $Log: eventlist.cpp,v $
// Revision 1.18  2001/10/22 15:24:48  McClean
// small designupdate
//
// Revision 1.17  2001/10/18 22:01:31  field
// kleiner Bugfix
//
// Revision 1.16  2001/10/18 14:31:23  field
// Scrollleisten :)
//
// Revision 1.15  2001/10/14 14:30:47  rasc
// -- EventList Darstellung ueberarbeitet
// -- kleiner Aenderungen und kleinere Bugfixes
// -- locales erweitert..
//
// Revision 1.14  2001/10/11 21:04:58  rasc
// - EPG:
//   Event: 2 -zeilig: das passt aber noch nicht  ganz (read comments!).
//   Key-handling etwas harmonischer gemacht  (Left/Right/Exit)
// - Code etwas restrukturiert und eine Fettnaepfe meinerseits beseitigt
//   (\r\n wg. falscher CSV Einstellung...)
//
// Revision 1.13  2001/10/04 19:28:44  fnbrd
// Eventlist benutzt ID bei zapit und laesst sich per rot wieder schliessen.
//
// Revision 1.12  2001/09/26 09:57:03  field
// Tontraeger-Auswahl ok (bei allen Chans. auf denen EPG geht)
//
// Revision 1.11  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.10  2001/09/20 17:02:16  field
// event-liste zeigt jetzt auch epgs an...
//
// Revision 1.9  2001/09/20 11:55:58  fnbrd
// removed warning.
//
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
void EventList::readEvents(unsigned onidSid, const std::string& channelname)
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
  if(zapit) {
    req.version = 2;
    req.command = sectionsd::allEventsChannelID;
    req.dataLength = 4;
    write(sock_fd, &req, sizeof(req));
    write(sock_fd, &onidSid, req.dataLength);
  }
  else {
    req.version = 2;
    req.command = sectionsd::allEventsChannelName;
    req.dataLength = strlen(channelname.c_str())+1;
    write(sock_fd, &req, sizeof(req));
    write(sock_fd, channelname.c_str(), req.dataLength);
  }
  sectionsd::msgResponseHeader resp;
  memset(&resp, 0, sizeof(resp));
  if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0) {
    close(sock_fd);
    return;
  }

    removeAllEvents(); // Alle gespeicherten Events loeschen
    current_event = (unsigned)-1;

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
        char tmpstr[256];
        char *actPos=pData;

        struct  tm tmZeit;
        struct  tm *tmZeit_now;
        int     evtTime, aktTime;
        time_t  tZeit  = time(NULL);
        tmZeit_now = localtime(&tZeit);
        aktTime = (tmZeit_now->tm_mon+ 1)* 1000000+ (tmZeit_now->tm_mday)* 10000+ (tmZeit_now->tm_hour)* 100+ tmZeit_now->tm_min;
        tmZeit = *tmZeit_now;

        while(*actPos && actPos<pData+resp.dataLength)
        {
            *epgID=0;
            actPos = copyStringto( actPos, epgID, sizeof(epgID), ' ');
            *edate=0;
            actPos = copyStringto( actPos, edate, sizeof(edate), ' ');
            *etime=0;
            actPos = copyStringto( actPos, etime, sizeof(etime), ' ');
            *eduration=0;
            actPos = copyStringto( actPos, eduration, sizeof(eduration), ' ');
            *ename=0;
            actPos = copyStringto( actPos, ename, sizeof(ename), '\n');

            event* evt = new event();


            sscanf(epgID, "%llx", &evt->epg.id);
            sscanf(edate, "%02d.%02d", &tmZeit.tm_mday, &tmZeit.tm_mon);
            tmZeit.tm_mon--;
            sscanf(etime, "%02d:%02d", &tmZeit.tm_hour, &tmZeit.tm_min);
            evtTime = (tmZeit.tm_mon+ 1)* 1000000+ (tmZeit.tm_mday)* 10000+ (tmZeit.tm_hour)* 100+ tmZeit.tm_min;
            tmZeit.tm_sec= 0;

            evt->epg.startzeit = mktime(&tmZeit);
//            printf("Time: %02d.%02d %02d:%02d %lx\n", tmZeit.tm_mday, tmZeit.tm_mon+ 1, tmZeit.tm_hour, tmZeit.tm_min, evt->startzeit);

            if ( (evtTime- aktTime) < 0 )
                current_event++;

            evt->epg.description=std::string(ename);

            // -- Create strings:   "Mon,  14:30, "      and   "23. Okt."
            // -- and  duration string "[999 min]"
            // -- rasc 2001-10-14

            // -- localized day   (strftime has to return english values!
            strftime(tmpstr,sizeof(tmpstr), "date.%a",&tmZeit);
            evt->datetime1_str = std::string(g_Locale->getText(tmpstr));
            strftime(tmpstr,sizeof(tmpstr), ". %H:%M, ",&tmZeit);
            evt->datetime1_str += std::string(tmpstr);

            strftime(tmpstr,sizeof(tmpstr), " %d. ",&tmZeit);
            evt->datetime2_str = std::string(tmpstr);
            strftime(tmpstr,sizeof(tmpstr), "date.%b",&tmZeit);
            evt->datetime2_str += std::string(g_Locale->getText(tmpstr));
            evt->datetime2_str += std::string(".");


            evt->duration_str=std::string("[");
            evt->duration_str+=std::string(eduration);
            evt->duration_str+=std::string(" min] ");



//            printf("id: %s - name: %s\n", epgID, evt->name.c_str());
//    tmp->number=number;
//    tmp->name=name;
            if(evt->epg.description !="")
                evtlist.insert(evtlist.end(), evt);
        }

        delete[] pData;
    }
    close(sock_fd);

    if ( current_event == (unsigned)-1 )
    {
        event* evt = new event();

        evt->epg.description= g_Locale->getText("epglist.noevents") ;
        evt->datetime1_str = std::string("");
        evt->datetime2_str = std::string("");
        evt->duration_str  = std::string("");
        evt->epg.id = 0;
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
//	height = 440;
	height = 480;
	theight= g_Fonts->eventlist_title->getHeight();
	fheight1= g_Fonts->eventlist_itemLarge->getHeight();
	{
	  int h1,h2; 
		h1 = g_Fonts->eventlist_itemSmall->getHeight();
		h2 = g_Fonts->eventlist_datetime->getHeight();
		fheight2 = (h1 > h2) ? h1 : h2;
	}
	fheight = fheight1 + fheight2 + 2;
	fwidth1 = g_Fonts->eventlist_datetime->getRenderWidth("DDD, 00:00,  ");
	fwidth2 = g_Fonts->eventlist_itemSmall->getRenderWidth("[999 min] ");


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

void EventList::exec(unsigned onidSid, const std::string& channelname)
{
  int key;
  name = channelname;
  paintHead();
  readEvents(onidSid, channelname);
  paint();
	
	int oldselected = selected;
	bool loop=true;
	while (loop)
	{
		key = g_RCInput->getKey(100);
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
			loop= false;
		}
        else if (key==CRCInput::RC_left)
        {
			loop= false;
		}
        else if (key==CRCInput::RC_red)
        {
			loop= false;
		}
   		else if (key==CRCInput::RC_help || key==CRCInput::RC_right)
		{
            event* evt = evtlist[selected];
            if ( evt->epg.id != 0 )
            {
    			hide();

    			g_EpgData->show("", 0, evt->epg.id, &evt->epg.startzeit);

                key = g_RCInput->getKey(100);
                if (key!=CRCInput::RC_red)
                    g_RCInput->pushbackKey(key);

    			paintHead();
    			paint();
            }
		}
	}

    hide();

}

void EventList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void EventList::paintItem(unsigned int pos)
{
    int color;
	int ypos = y+ theight+0 + pos*fheight;

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}
    else if (liststart+pos == current_event )
	{
		color = COL_MENUCONTENT+ 1; //COL_MENUCONTENTINACTIVE+ 4;
	}
    else
   	{
		color = COL_MENUCONTENT;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

	if(liststart+pos<evtlist.size())
	{
		event* evt = evtlist[liststart+pos];

//		printf("Rendering '%s'\n", evt->name.c_str());
//		printf("date time duration '%s'\n", evt->datetimeduration.c_str());


//$$$ RASC: das color +1 ist falsch hier (Absicht, damit man sieht hier muss was getan werden)
//Behoben der Fontrenderer kommt jetzt damit zurecht

//$$$ auch sollten wg. der besseren Darstellung andere Fontmappings benutzt werden...

		//  datetime1_str  datetime2_str    duration_str
		//  evt->epg.description

		// 1st line
		g_Fonts->eventlist_datetime->RenderString(x+5,         ypos+ fheight1+3, fwidth1+5,
			evt->datetime1_str.c_str(), color);
		g_Fonts->eventlist_datetime->RenderString(x+5+fwidth1, ypos+ fheight1+3, width-fwidth1-10- 20,
			evt->datetime2_str.c_str(), color);
		g_Fonts->eventlist_itemSmall->RenderString(x+width-fwidth2-5- 20, ypos+ fheight1+3, fwidth2,
			evt->duration_str.c_str(), color);
		// 2nd line
		g_Fonts->eventlist_itemLarge->RenderString(x+ 20, ypos+ fheight, width- 25- 20,
			evt->epg.description.c_str(), color);
	}
}

void EventList::paintHead()
{
    char l_name[100];
    snprintf(l_name, sizeof(l_name), g_Locale->getText("epglist.head").c_str(), name.c_str() );

	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width, l_name, COL_MENUHEAD);
}

void EventList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

    int ypos = y+ theight;
    int sb = fheight* listmaxshow;
    g_FrameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

    int sbc= ((evtlist.size()- 1)/ listmaxshow)+ 1;
    float sbh= (sb- 4)/ sbc;
    int sbs= (selected/listmaxshow);

    g_FrameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}


