//
// $Id: epgview.cpp,v 1.14 2001/09/22 13:18:07 field Exp $
//
// $Log: epgview.cpp,v $
// Revision 1.14  2001/09/22 13:18:07  field
// epg-anzeige bug gefixt
//
// Revision 1.13  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.12  2001/09/20 17:02:16  field
// event-liste zeigt jetzt auch epgs an...
//
// Revision 1.11  2001/09/20 13:44:57  field
// epg-Anzeige verbessert
//
// Revision 1.10  2001/09/16 03:38:44  McClean
// i18n + small other fixes
//
// Revision 1.9  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.8  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.7  2001/09/03 03:34:04  tw-74
// cosmetic fixes, own "Mg" fontmetrics
//
// Revision 1.6  2001/08/20 13:07:10  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.5  2001/08/16 00:19:44  fnbrd
// Removed debug output.
//
//

#include "epgdata.h"
#include "../global.h"

static char* ocopyStringto(const char* from, char* to, int len)
{
	const char *fromend=from+len;
	while(*from!='\xff' && from<fromend)
		*(to++)=*(from++);
	*to=0;
	return (char *)++from;
}

CEpgData::CEpgData()
{
}

void CEpgData::start()
{
	oy = 290;

	topheight= g_Fonts->epg_title->getHeight();
	topboxheight=topheight+6;
	botheight=g_Fonts->epg_date->getHeight();
	botboxheight=botheight+6;
	medlineheight=g_Fonts->epg_info1->getHeight();
	medlinecount=(oy-topboxheight-botboxheight)/medlineheight;

	oy=topboxheight+botboxheight+medlinecount*medlineheight; // recalculate

	sy = (((g_settings.screen_EndY-g_settings.screen_StartY)-oy) / 2) + g_settings.screen_StartY;
}


void CEpgData::addTextToArray( string text  )
{
	//printf("line: >%s<\n", text.c_str() );
	if (text==" ")
	{
		emptyLineCount ++;
		if(emptyLineCount<2)
		{
			epgText.insert(epgText.end(), text );
		}
	}
	else
	{
		emptyLineCount = 0;
		epgText.insert(epgText.end(), text );
	}
}

void CEpgData::processTextToArray( char* text  )
{
	string	aktLine = "";
	string	aktWord = "";
	int	aktWidth = 0;
    strcat(text, " ");

	//printf("orginaltext:\n%s\n\n", text);
	while(*text!=0)
	{
		if ( (*text==' ') || (*text=='\n') || (*text=='-') || (*text=='.') )
		{
			//check the wordwidth - add to this line if size ok
            if(*text=='\n')
			{	//enter-handler
				//printf("enter-");
				addTextToArray( aktLine );
				aktLine = "";
				aktWidth= 0;
			}
            else
            {
    			aktWord += *text;

    			int aktWordWidth = g_Fonts->epg_info2->getRenderWidth(aktWord.c_str());
    			if((aktWordWidth+aktWidth)<(ox-20))
    			{//space ok, add
				    aktWidth += aktWordWidth;
    				aktLine += aktWord;
    			}
    			else
    			{//new line needed
				    addTextToArray( aktLine );
    				aktLine = aktWord;
    				aktWidth = aktWordWidth;
    			}
    			aktWord = "";
            }
		}
		else
		{
			aktWord += *text;
		}
		text++;
	}
	//add the rest
	addTextToArray( aktLine + aktWord );
}

void CEpgData::showText( int startPos, int ypos )
{
	int textCount = epgText.size();
	int y=ypos;
	int linecount=medlinecount;
	string t;
	g_FrameBuffer->paintBoxRel(sx, y, ox, linecount*medlineheight, COL_MENUCONTENT);

	for(int i=startPos; i<textCount && i<startPos+linecount; i++,y+=medlineheight)
	{
		t=epgText[i];
        if ( i< info1_lines )
            g_Fonts->epg_info1->RenderString(sx+10, y+medlineheight, ox-15, t.c_str(), COL_MENUCONTENT);
        else
    		g_Fonts->epg_info2->RenderString(sx+10, y+medlineheight, ox-15, t.c_str(), COL_MENUCONTENT);
	}
}

void CEpgData::show( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit )
{
	int height;
	height = g_Fonts->epg_date->getHeight();
	g_FrameBuffer->paintBoxRel(g_settings.screen_StartX, g_settings.screen_StartY, 50, height+5, COL_INFOBAR);
	g_Fonts->epg_date->RenderString(g_settings.screen_StartX+10, g_settings.screen_StartY+height, 40, "-@-", COL_INFOBAR);

	GetEPGData( channelName, onid_tsid, id, startzeit );
	g_FrameBuffer->paintBoxRel(g_settings.screen_StartX, g_settings.screen_StartY, 50, height+5, COL_BACKGROUND);

	if(strlen(epgData.title)==0)
	{
		//no epg-data found :(
		char *text = (char*) g_Locale->getText("epgviewer.notfound").c_str();
		int oy = 30;
		int ox = g_Fonts->epg_info2->getRenderWidth(text)+30;
		int sx = (((g_settings.screen_EndX- g_settings.screen_StartX)-ox) / 2) + g_settings.screen_StartX;
		int sy = (((g_settings.screen_EndY- g_settings.screen_StartY)-oy) / 2) + g_settings.screen_StartY;
		height = g_Fonts->epg_info2->getHeight();
		g_FrameBuffer->paintBoxRel(sx, sy, ox, height+10, COL_MENUCONTENT);
		g_Fonts->epg_info2->RenderString(sx+15, sy+height+5, ox-15, text, COL_MENUCONTENT);
		g_RCInput->getKey(20);
		g_FrameBuffer->paintBoxRel(sx, sy, ox, height+10, COL_BACKGROUND);
		return;
	}

    // Variable Breite, falls der Text zu lang' ist...
    ox = g_Fonts->epg_title->getRenderWidth( epgData.title )+ 15;
    if (ox < 500 )
        ox = 500;
    else if ( ox > ( g_settings.screen_EndX- g_settings.screen_StartX - 20) )
        ox = ( g_settings.screen_EndX- g_settings.screen_StartX - 20);

	sx = (((g_settings.screen_EndX-g_settings.screen_StartX) -ox) / 2) + g_settings.screen_StartX;

	if(strlen(epgData.info1)!=0)
	{
		processTextToArray( epgData.info1 );
	}
    info1_lines = epgText.size();

	//scan epg-data - sort to list
	if ( ( strlen(epgData.info2)==0 ) && (info1_lines == 0) )
	{
        strcpy(epgData.info2, g_Locale->getText("epgviewer.nodetailed").c_str());
    }

    processTextToArray( epgData.info2 );

	//show the epg
	g_FrameBuffer->paintBoxRel(sx, sy, ox, topboxheight, COL_MENUHEAD);
	g_Fonts->epg_title->RenderString(sx+10, sy+topheight+3, ox-15, epgData.title, COL_MENUHEAD);

	//show date-time....
	g_FrameBuffer->paintBoxRel(sx, sy+oy-botboxheight, ox, botboxheight, COL_MENUHEAD);
	char fromto[40];
	int widthl,widthr;
	strcpy(fromto,epgData.start); strcat(fromto," - "); strcat(fromto,epgData.end);
	widthl = g_Fonts->epg_date->getRenderWidth(fromto);
	g_Fonts->epg_date->RenderString(sx+10,  sy+oy-3, widthl, fromto, COL_MENUHEAD);
	widthr = g_Fonts->epg_date->getRenderWidth(epgData.date);
	g_Fonts->epg_date->RenderString(sx+ox-10-widthr,  sy+oy-3, widthr, epgData.date, COL_MENUHEAD);

	int showPos = 0;
	int textCount = epgText.size();
	int textypos = sy+topboxheight;
	showText(showPos, textypos);

	//show progressbar
    if ( strlen(epgData.done)!= 0 )
    {
    	int progress = atoi(epgData.done);
    	int pbx = sx + 10 + widthl + 10 + ((ox-104-widthr-widthl-10-10-20)>>1);
    	g_FrameBuffer->paintBoxRel(pbx, sy+oy-height, 104, height-6, COL_MENUHEAD+7);
    	g_FrameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, 100, height-10, COL_MENUHEAD+2);
    	g_FrameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, progress, height-10, COL_MENUHEAD+5);
    }

	bool loop=true;
	int scrollCount;
	while(loop)
	{
		int key = g_RCInput->getKey(40);

		scrollCount = medlinecount;
		if(showPos==0)	//titleinfo exists
			scrollCount--;

		if (key==CRCInput::RC_down)
		{
			if(showPos+scrollCount<textCount)
			{
				showPos += scrollCount;
				showText(showPos,textypos);
			}
		}
		else if (key==CRCInput::RC_up)
		{
			showPos -= scrollCount;
			bool toShow = true;
			if(showPos<0)
			{
				showPos = 0;
				toShow = false;
			}
			if((showPos==0) && (scrollCount==medlinecount))
			{
				toShow = true;
			}
			if (toShow)
				showText(showPos,textypos);
		}
		else if ( (key==CRCInput::RC_ok) || (key==CRCInput::RC_help)  || (key==g_settings.key_channelList_cancel) )
		{
			loop = false;
		}
	}
	hide();
}

void CEpgData::hide()
{
	g_FrameBuffer->paintBoxRel (sx, sy, ox+10, oy+10, 255);
}

void CEpgData::GetEPGData( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit )
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	epgText.clear();
	emptyLineCount = 0;
	strcpy(epgData.title,"");
	strcpy(epgData.info1,"");
	strcpy(epgData.info2,"");
	strcpy(epgData.date,"");
	strcpy(epgData.start,"");
	strcpy(epgData.end,"");
	strcpy(epgData.done,"");

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(sectionsd::portNumber);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to server!");
		return;
	}

	#ifdef EPG_SECTIONSD
		//use new sectionsd-daemon

    if ( (( onid_tsid != 0 ) && ( g_settings.epg_byname == 0 ) )||
         ( id!= 0 ) )
    {
        if ( id!= 0 )
        {
            // query EPG für bestimmtes Event
    		sectionsd::msgRequestHeader req;
    		req.version = 2;
    		req.command = sectionsd::epgEPGid;
    		req.dataLength = 12;
    		write(sock_fd,&req,sizeof(req));

            write(sock_fd, &id, sizeof(id));
            write(sock_fd, startzeit, sizeof(*startzeit));
            printf("query epg for evt_id >%llx<, time %lx\n", id, *startzeit);
        }
        else
        {
            // query EPG normal
    		sectionsd::msgRequestHeader req;
    		req.version = 2;
    		req.command = sectionsd::actualEPGchannelID;
    		req.dataLength = 4;
    		write(sock_fd,&req,sizeof(req));

            write(sock_fd, &onid_tsid, sizeof(onid_tsid));
            printf("query epg for onid_tsid >%x< (%s)\n", onid_tsid, channelName.c_str());
        }

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
			char* pData = new char[nBufSize] ;
			read(sock_fd, pData, nBufSize);

            unsigned long long          tmp_id;
            sectionsd::sectionsdTime    epg_times;
            char* dp = pData;

            sscanf(dp, "%012llx\xFF", &tmp_id);
            dp+= 13;
            dp = ocopyStringto( dp, epgData.title, sizeof(epgData.title) );
            dp = ocopyStringto( dp, epgData.info1, sizeof(epgData.info1) );
			dp = ocopyStringto( dp, epgData.info2, sizeof(epgData.info2) );
            sscanf(dp, "%08lx\xFF%08x\xFF", &epg_times.startzeit, &epg_times.dauer );

            struct tm *pStartZeit = localtime(&epg_times.startzeit);
            int nSDay(pStartZeit->tm_mday), nSMon(pStartZeit->tm_mon+1), nSYear(pStartZeit->tm_year+1900),
                nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);
            sprintf( epgData.date, "%02d.%02d.%04d", nSDay, nSMon, nSYear );
            sprintf( epgData.start, "%02d:%02d", nSH, nSM );

            long int uiEndTime(epg_times.startzeit+ epg_times.dauer);
            struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
            int nFH(pEndeZeit->tm_hour), nFM(pEndeZeit->tm_min);
            sprintf( epgData.end, "%02d:%02d", nFH, nFM );


            if (( time(NULL)- epg_times.startzeit )>= 0 )
            {
                unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-epg_times.startzeit)/(float)epg_times.dauer*100.);
                if (nProcentagePassed<= 100)
                    sprintf( epgData.done, "%03u", nProcentagePassed );
            }

			delete[] pData;
		}
    }
    else
    {
		sectionsd::msgRequestHeader req;
		req.version = 2;
		req.command = sectionsd::actualEPGchannelName;
		req.dataLength = channelName.length()+1;
		write(sock_fd,&req,sizeof(req));

		char chanName[50];
		strcpy(chanName, channelName.c_str());
		for(int count=strlen(chanName)-1;count>=0;count--)
		{
			if((chanName[count]==' ') || (chanName[count]==0))
			{
				chanName[count]=0;
			}
			else
				break;
		}
		printf("query epg for >%s<\n", chanName);
		write(sock_fd, chanName, strlen(chanName)+1);

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
//			printf("neutrino nBufsize: %d\n", nBufSize);
			char* pData = new char[nBufSize] ;
			read(sock_fd, pData, nBufSize);
//			printf("neutrino epgdata: \n%s\n", pData);

			char tmp[20];
			char *pos = ocopyStringto( pData, tmp, sizeof(tmp));
//			printf("id: %s\n", tmp);
			pos = ocopyStringto( pos, epgData.title, sizeof(epgData.title));
//			printf("title: %s\n", epgData.title);
			pos = ocopyStringto( pos, epgData.info1, sizeof(epgData.info1) );
			pos = ocopyStringto( pos, epgData.info2, sizeof(epgData.info2));
			pos = ocopyStringto( pos, epgData.date, sizeof(epgData.date));
			pos = ocopyStringto( pos, epgData.start, sizeof(epgData.start));
			pos = ocopyStringto( pos, epgData.end, sizeof(epgData.end));
			pos = ocopyStringto( pos, epgData.done, sizeof(epgData.done));

			delete[] pData;
//			printf("copied\n");
		}
    }
	#else
		//for old epgd users
		struct  msgEPGRequest
		{
		    char version;
		    char cmd;
		    char Name[50];
		} req;

		req.version = 1;
		req.cmd     = 1;
		strcpy( req.Name, channelName.c_str() );

		write(sock_fd,&req,sizeof(req));

		struct msgEPGResponse
		{
				char version;
				char sizeOfBuffer[6];
				char pEventBuffer[1];
		} rep;

		read(sock_fd, &rep, sizeof(rep.version)+sizeof(rep.sizeOfBuffer));
		int nBufSize = atol(rep.sizeOfBuffer);
		char* pData = new char[nBufSize+1] ;

		read(sock_fd, pData, nBufSize);

//		printf("neutrino epgdata: %i: %s\n", rep.version, rep.sizeOfBuffer);

		if( nBufSize > 0 )
		{
			printf("%s",pData);
			char *pos = ocopyStringto( pData, epgData.title, sizeof(epgData.title));
			pos = ocopyStringto( pos, epgData.info1, sizeof(epgData.info1) );
			pos = ocopyStringto( pos, epgData.info2, sizeof(epgData.info2));
			pos = ocopyStringto( pos, epgData.date, sizeof(epgData.date));
			pos = ocopyStringto( pos, epgData.start, sizeof(epgData.start));
			pos = ocopyStringto( pos, epgData.end, sizeof(epgData.end));
			pos = ocopyStringto( pos, epgData.done, sizeof(epgData.done));
		}
		delete[] pData;
	#endif

	printf("exit epg-get\n\n");
	close(sock_fd);
}

