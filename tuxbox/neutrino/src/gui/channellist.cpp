//
// $Id: channellist.cpp,v 1.33 2001/10/16 18:34:13 rasc Exp $
//
// $Log: channellist.cpp,v $
// Revision 1.33  2001/10/16 18:34:13  rasc
// -- QuickZap to last channel verbessert.
// -- Standard Kanal muss ca. 2-3 Sekunden aktiv sein fuer LastZap Speicherung.
// -- eigene Klasse fuer die Channel History...
//
// Revision 1.32  2001/10/13 00:46:48  McClean
// nstreamzapd-support broken - repaired
//
// Revision 1.31  2001/10/11 21:04:58  rasc
// - EPG:
//   Event: 2 -zeilig: das passt aber noch nicht  ganz (read comments!).
//   Key-handling etwas harmonischer gemacht  (Left/Right/Exit)
// - Code etwas restrukturiert und eine Fettnaepfe meinerseits beseitigt
//   (\r\n wg. falscher CSV Einstellung...)
//
// Revision 1.30  2001/10/10 17:17:13  field
// zappen auf onid_sid umgestellt
//
// Revision 1.29  2001/10/02 17:56:33  McClean
// time in infobar (thread probs?) and "0" quickzap added
//
// Revision 1.28  2001/09/27 17:19:21  field
// Numeric-Zap fix gefixt
//
// Revision 1.27  2001/09/27 11:23:51  field
// Numzap gefixt, kleiner Bugfixes
//
// Revision 1.26  2001/09/26 22:11:08  rasc
// - kleiner Bugfix bei Abort NumericChannelzap
//
// Revision 1.25  2001/09/26 16:24:17  rasc
// - kleinere Aenderungen: Channel Num Zap fuer >999 Channels
//   (Eutelsat/Astra) und eigener Font
//
// Revision 1.24  2001/09/23 21:34:07  rasc
// - LIFObuffer Module, pushbackKey fuer RCInput,
// - In einige Helper und widget-Module eingebracht
//   ==> harmonischeres Menuehandling
// - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)
//
// Revision 1.23  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.22  2001/09/20 19:21:37  fnbrd
// Channellist mit IDs.
//
// Revision 1.21  2001/09/20 14:10:10  field
// neues EPG-Handling abschaltbar
//
// Revision 1.20  2001/09/20 13:44:57  field
// epg-Anzeige verbessert
//
// Revision 1.19  2001/09/20 00:36:32  field
// epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
// Revision 1.18  2001/09/18 11:48:43  fnbrd
// Changed some parameter to const string&
//
// Revision 1.17  2001/09/17 12:45:12  field
// Sprache online umstellbar, kleine Aufraeumarbeiten
//
// Revision 1.16  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.15  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.14  2001/09/06 19:13:21  McClean
// no changes
//
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
#include "../global.h"

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

//printf("\n START CChannelList::updateEvents \n\n");
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

    if ( ( zapit ) &&  ( g_settings.epg_byname == 0 ) )
        req.command = sectionsd::actualEventListTVshortIDs;
    else
        req.command = sectionsd::actualEventListTVshort;
    req.dataLength = 0;
    write(sock_fd,&req,sizeof(req));

    sectionsd::msgResponseHeader resp;
    memset(&resp, 0, sizeof(resp));
    if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
    {
        close(sock_fd);
        return;
    }
    if(resp.dataLength<=0)
    {
        close(sock_fd);
        return;
    }

    char* pData = new char[resp.dataLength] ;
    if ( read(sock_fd, pData, resp.dataLength)<=0 )
    {
        delete[] pData;
        close(sock_fd);
        return;
    }

    close(sock_fd);

    char *actPos = pData;

    for(unsigned int count=0;count<chanlist.size();count++)
        chanlist[count]->currentEvent.description="";

//printf("\n read finished CChannelList::updateEvents \n\n");

    if ( req.command == sectionsd::actualEventListTVshortIDs )
    {
        printf("data length: %u\n", resp.dataLength);
        while(actPos<pData+resp.dataLength)
        {
            unsigned* serviceID = (unsigned*) actPos;
            actPos+=4;

            unsigned long long* evt_id = (unsigned long long*) actPos;
            actPos+=8;

            // quick'n dirty, sollte man mal anders machen
            for (unsigned int count=0;count<chanlist.size();count++)
            {
                if (chanlist[count]->onid_sid==*serviceID)
                {
                    chanlist[count]->currentEvent.id= *evt_id;
                    chanlist[count]->currentEvent.description= actPos;
                    //	printf("Channel found: %s\n", actPos);
                    break;
                }
            }
            actPos+=strlen(actPos)+1;
        }
    } // if sectionsd::actualEventListTVshortIDs 
    else
    {
        // old way
        char epgID[20];
        char channelName[50];
        char channelDescription[1000];
        while (*actPos && actPos<pData+resp.dataLength)
        {
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
            for(unsigned int count=0;count<chanlist.size();count++)
            {
                if(!strcasecmp(chanlist[count]->name.c_str(), channelName))
                {
                    chanlist[count]->currentEvent.description = channelDescription;
                    //	printf("Channel found\n");
                	break;
                }
            }
        }
    } // else zapit
    delete[] pData;
//printf("\n END CChannelList::updateEvents \n\n");
    return;
}

CChannelList::CChannelList(int Key=-1, const std::string &Name)
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

CChannelList::~CChannelList()
{
	for(unsigned int count=0;count<chanlist.size();count++)
	{
		delete chanlist[count];
	}
	chanlist.clear();
}

void CChannelList::addChannel(int key, int number, const std::string& name, unsigned int ids)
{
	channel* tmp = new channel();
	tmp->key=key;
	tmp->number=number;
	tmp->name=name;
    tmp->onid_sid=ids;
	chanlist.insert(chanlist.end(), tmp);
}

void CChannelList::setName(const std::string& Name)
{
	name = Name;
}

int CChannelList::getKey(int id)
{
	return chanlist[id]->key;
}

const std::string& CChannelList::getActiveChannelName()
{
	return chanlist[selected]->name;
}

int CChannelList::getActiveChannelNumber()
{
	return selected+1;
}

void CChannelList::exec()
{
	if(chanlist.size()==0)
	{
		//evtl. anzeige dass keine kanalliste....
		return;
	}
	paintHead();
	updateEvents();
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
			if (selected>chanlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (key==g_settings.key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=chanlist.size()-1;
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
				selected = chanlist.size()-1;
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
			selected = (selected+1)%chanlist.size();
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
			g_RCInput->pushbackKey (key);
			loop=false;
		}
	}
	hide();
	if(zapOnExit)
	{
		zapTo(selected);
	}
}

void CChannelList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

bool CChannelList::showInfo(int pos)
{
	if((pos >= (signed int) chanlist.size()) || (pos<0))
	{
		return false;
	}
	selected=pos;
	channel* chan = chanlist[selected];
	g_InfoViewer->showTitle(selected+1, chan->name, chan->onid_sid, true);
	return true;
}

void CChannelList::zapTo(int pos)
{
	if ( (pos >= (signed int) chanlist.size()) || (pos< 0) )
	{
		return;
	}
	selected= pos;
  	channel* chan = chanlist[selected];
	lastChList.store (selected);

	if ( pos!=(int)tuned )
	{
		tuned = pos;

		if (!g_RemoteControl->getZapper())
		{
			g_RemoteControl->zapTo( chan->name );
		}
		else
		{	//zapit-mode
			if ( g_settings.epg_byname == 0 )
				g_RemoteControl->zapTo_onid_sid( chan->onid_sid );
			else
				g_RemoteControl->zapTo( chan->name );

		}
	}
	g_InfoViewer->showTitle(selected+ 1, chan->name, chan->onid_sid);
}

void CChannelList::numericZap(int key)
{
    if(chanlist.size()==0)
    {
        //evtl. anzeige dass keine kanalliste....
        return;
    }
 
	//schneller zap mit "0" taste zwischen den letzten beiden sendern...
	if(key==0) {
	  	int  ch;

		if( (ch=lastChList.getlast(1)) != -1) {
			if ((unsigned int)ch != tuned) {
				printf("quicknumtune(0)\n");
				lastChList.clear_storedelay (); // ignore store delay
				zapTo(ch);		        // zap to last
			}
		}
		return;
	}

	int ox=300;
    int oy=200;
	int sx = g_Fonts->channel_num_zap->getRenderWidth("0000")+14;
    int sy = g_Fonts->channel_num_zap->getHeight()+6;
	char valstr[10];
	int chn=key;
	int pos=1;

	while(1)
	{
		sprintf((char*) &valstr, "%d", chn);
		while(strlen(valstr)<4)
		{
 			strcat(valstr,"-");
		}
		g_FrameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR);
		g_Fonts->channel_num_zap->RenderString(ox+7, oy+sy-3, sx, valstr, COL_INFOBAR);

        showInfo(chn- 1);

		if ( ( key=g_RCInput->getKey(30) ) == CRCInput::RC_timeout )
        {
            if ( ( chn > (int)chanlist.size() ) || (chn == 0) )
                chn = tuned + 1;
  			break;
        }
		else if ( (key>=0) && (key<=9) )
		{ //numeric
            if ( pos==4 )
            {
                chn = key;
                pos = 0;
            }
            else
    			chn = chn* 10 + key;

			pos++;
		}
		else if (key==CRCInput::RC_ok)
		{
            if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
            {
        		chn = tuned + 1;
	        }
			break;
		}
        else if (key==g_settings.key_quickzap_down)
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else
            {
				chn--;

                if (chn > (int)chanlist.size())
                    chn = (int)chanlist.size();
			}
		}
		else if (key==g_settings.key_quickzap_up)
		{
			chn++;

            if (chn > (int)chanlist.size())
                chn = 1;
        }
        else if (key==CRCInput::RC_home || key==CRCInput::RC_left || key==CRCInput::RC_right)
        {
            // Abbruch ohne Channel zu wechseln

            chn = tuned + 1;
            break;
		};
	}

	g_FrameBuffer->paintBoxRel(ox, oy, sx, sy, COL_BACKGROUND);

	chn--;
	if (chn<0)
		chn=0;
	zapTo( chn );
}

void CChannelList::quickZap(int key)
{
    if(chanlist.size()==0)
    {
        //evtl. anzeige dass keine kanalliste....
        return;
    }
 
//	printf("quickzap start\n");
    if (key==g_settings.key_quickzap_down)
    {
        if(selected==0)
            selected = chanlist.size()-1;
        else
            selected--;
//				channel* chan = chanlist[selected];
    }
    else if (key==g_settings.key_quickzap_up)
    {
        selected = (selected+1)%chanlist.size();
//			channel* chan = chanlist[selected];
    };

    zapTo( selected );
}

void CChannelList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width, fheight, color);
	if(liststart+pos<chanlist.size())
	{
		channel* chan = chanlist[liststart+pos];
		//number
                char tmp[10];
                sprintf((char*) tmp, "%d", chan->number);
		int numpos = x+5+numwidth- g_Fonts->channellist_number->getRenderWidth(tmp);
		g_Fonts->channellist_number->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);
		if(strlen(chan->currentEvent.description.c_str()))
		{
    			// name + description
			char nameAndDescription[100];
			snprintf(nameAndDescription, sizeof(nameAndDescription), "%s - %s", chan->name.c_str(), chan->currentEvent.description.c_str());
			g_Fonts->channellist->RenderString(x+5+numwidth+10,ypos+fheight, width-numwidth-20, nameAndDescription, color);
                }
		else
		  //name
		  g_Fonts->channellist->RenderString(x+5+numwidth+10,ypos+fheight, width-numwidth-20, chan->name.c_str(), color);
	}
}

void CChannelList::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(name).c_str(), COL_MENUHEAD);
}

void CChannelList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  chanlist[liststart]->number + listmaxshow;

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

