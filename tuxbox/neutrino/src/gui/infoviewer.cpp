//
// $Id: infoviewer.cpp,v 1.24 2001/09/20 14:10:10 field Exp $
//
// $Log: infoviewer.cpp,v $
// Revision 1.24  2001/09/20 14:10:10  field
// neues EPG-Handling abschaltbar
//
// Revision 1.23  2001/09/20 11:56:00  field
// Final fix for new structure
//
// Revision 1.22  2001/09/20 11:37:29  fnbrd
// Fixed small bug.
//
// Revision 1.21  2001/09/20 11:21:06  field
// buggy...
//
// Revision 1.20  2001/09/20 00:36:32  field
// epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
// Revision 1.19  2001/09/19 18:03:14  field
// Infobar, Sprachauswahl
//
// Revision 1.18  2001/09/18 20:20:26  field
// Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
// vorbereitet
//
// Revision 1.17  2001/09/17 12:45:12  field
// Sprache online umstellbar, kleine Aufraeumarbeiten
//
// Revision 1.16  2001/09/16 02:27:22  McClean
// make neutrino i18n
//
// Revision 1.15  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.14  2001/09/13 17:15:51  field
// verbessertes warten aufs epg
//
// Revision 1.13  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.12  2001/09/09 23:53:46  fnbrd
// Fixed some bugs, only shown compiling with -Os.
// Conclusion: use -Os ;)
//
// Revision 1.11  2001/09/05 21:21:16  McClean
// design-fix
//
// Revision 1.10  2001/08/22 11:29:31  McClean
// infoviewer designfix
//
// Revision 1.9  2001/08/20 13:07:10  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.8  2001/08/16 23:19:18  McClean
// epg-view and quickview changed
//
// Revision 1.7  2001/08/16 00:19:44  fnbrd
// Removed debug output.
//
//

#include "infoviewer.h"
#include "../global.h"

CInfoViewer::CInfoViewer()
{
	intShowDuration = 15; //7,5 sec
    BoxStartX= BoxStartY= BoxEndX= BoxEndY=0;

	strcpy( running, "");
	strcpy( next, "");
	strcpy( runningStart, "");
	strcpy( nextStart, "");
	strcpy( runningDuration, "");
	strcpy( nextDuration, "");

	runningPercent = 0;
	CurrentChannel = "";

    pthread_cond_init( &epg_cond, NULL );
    pthread_mutex_init( &epg_mutex, NULL );

    if (pthread_create (&thrViewer, NULL, InfoViewerThread, (void *) this) != 0 )
	{
		perror("CInfoViewer::CInfoViewer create thrViewer failed\n");
	}

    pthread_cond_init( &lang_cond, NULL );

    if (pthread_create (&thrLangViewer, NULL, LangViewerThread, (void *) this) != 0 )
	{
		perror("CInfoViewer::CInfoViewer create thrLangViewer failed\n");
	}

}

void CInfoViewer::start()
{
	InfoHeightY = g_Fonts->infobar_number->getHeight()*9/8 +
                  2*g_Fonts->infobar_info->getHeight() +
                  25;
    InfoHeightY_Info = g_Fonts->infobar_small->getHeight()+ 5;

    ChanWidth = g_Fonts->infobar_number->getRenderWidth("000") + 10;
	ChanHeight = g_Fonts->infobar_number->getHeight()*9/8;

}

void CInfoViewer::setDuration( int Duration )
{
	intShowDuration = Duration;
}

void CInfoViewer::showTitle( int ChanNum, string Channel, unsigned int onid_tsid, bool CalledFromNumZap )
{
    pthread_mutex_lock( &epg_mutex );

	CurrentChannel = Channel;
    Current_onid_tsid = onid_tsid;

//  Auskommentieren, falls es euch nicht gef„llt..?
    ShowInfo_Info = !CalledFromNumZap;
//    ShowInfo_Info = false;

    if ( CalledFromNumZap )
        EPG_NotFound_Text = (char*) g_Locale->getText("infoviewer.epgnotload").c_str();
    else
        EPG_NotFound_Text =  (char*) g_Locale->getText("infoviewer.epgwait").c_str();
    is_visible = true;

    pthread_mutex_unlock( &epg_mutex );

	BoxStartX = g_settings.screen_StartX+ 20;
	BoxEndX   = g_settings.screen_EndX- 20;
	BoxEndY   = g_settings.screen_EndY- 20;


    if ( ShowInfo_Info )
        BoxStartY = BoxEndY- InfoHeightY- InfoHeightY_Info+ 6;
    else
    	BoxStartY = BoxEndY- InfoHeightY;

	//frameBuffer->paintVLine(settings->screen_StartX,0,576, 3);
	//frameBuffer->paintVLine(settings->screen_EndX,0,576, 3);
	//frameBuffer->paintHLine(0,719, settings->screen_EndY,3);


    g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth >>1), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);

	//number box
	g_FrameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW);
	g_FrameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, COL_INFOBAR);

	//channel number
	char strChanNum[10];
	sprintf( (char*) strChanNum, "%d", ChanNum);
	int ChanNumXPos = BoxStartX + ((ChanWidth - g_Fonts->infobar_number->getRenderWidth(strChanNum))>>1);
	g_Fonts->infobar_number->RenderString(ChanNumXPos, BoxStartY+ChanHeight, ChanWidth, strChanNum, COL_INFOBAR);

	//infobox
	int ChanNameX = BoxStartX + ChanWidth + 10;
	int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?
	g_FrameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndY, COL_INFOBAR);
	
	// ... with channel name
	int height=g_Fonts->infobar_channame->getHeight()+5;
	g_Fonts->infobar_channame->RenderString(ChanNameX+ 20, ChanNameY+height, BoxEndX-ChanNameX- 140, Channel.c_str(), COL_INFOBAR);

	ChanInfoX = BoxStartX + (ChanWidth >>1);
	int ChanInfoY = BoxStartY + ChanHeight+10;
	g_FrameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndY, COL_INFOBAR);

    if ( ShowInfo_Info )
    {
        ButtonWidth = (BoxEndX- ChanInfoX)>> 2;

        g_FrameBuffer->paintBackgroundBox(ChanInfoX, BoxEndY- InfoHeightY_Info, BoxEndX, BoxEndY- InfoHeightY_Info+ 1);

        g_FrameBuffer->paintIcon("blau.raw", BoxEndX- ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
        g_Fonts->infobar_small->RenderString(BoxEndX- ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 31, g_Locale->getText("infoviewer.streaminfo").c_str(), COL_INFOBAR);

        g_FrameBuffer->paintIcon("rot.raw", BoxEndX- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
        g_Fonts->infobar_small->RenderString(BoxEndX- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR);
    }

    pthread_cond_signal( &epg_cond );
    pthread_cond_signal( &lang_cond );

    usleep(50);

    int key;

    if ( !CalledFromNumZap )
    {
        do
        {
            key = g_RCInput->getKey( intShowDuration* 5 );

            if ( key == CRCInput::RC_blue )
            {
                g_StreamInfo->exec(NULL, "");
                key = CRCInput::RC_timeout;
            }
// Auskommentiert - wird von Hauptschleife aufgerufen...?
/*            else if ( key == CRCInput::RC_yellow )
            {
                killTitle();
                g_EventList->exec(Channel);
                key = CRCInput::RC_timeout;
            }
*/
        } while (false);

        if ( ( key != CRCInput::RC_timeout ) &&
             ( ( key != CRCInput::RC_ok ) || ( CalledFromNumZap ) ) &&
             ( ( key != CRCInput::RC_home ) || ( CalledFromNumZap ) ) )
        {
            g_RCInput->addKey2Buffer(key);
        };

        if ( ( key != g_settings.key_quickzap_up ) &&
             ( key != g_settings.key_quickzap_down ) &&
             ( key != CRCInput::RC_help ) &&
             ( !CalledFromNumZap ) )
        {
            killTitle();
        };
    };
}


void CInfoViewer::showButtons()
{
    // welche Bedingung auch immer fr die gelbe Taste...?
    if ( false )
    {
        g_FrameBuffer->paintIcon("gelb.raw", BoxEndX- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
        g_Fonts->infobar_small->RenderString(BoxEndX- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR);
    };

    // grn, wenn mehrere APIDs
    if ( g_RemoteControl->apid_info.count_apids> 1 )
    {
        g_FrameBuffer->paintIcon("gruen.raw", BoxEndX- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
        g_Fonts->infobar_small->RenderString(BoxEndX- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR);
    }
}

void CInfoViewer::showData()
{
	int height;
	int ChanNameY = BoxStartY + (ChanHeight>>1)+3;
	int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10
	

	//percent
	height = g_Fonts->infobar_channame->getHeight()/3;
	int height2= int( g_Fonts->infobar_channame->getHeight()/1.5);
	g_FrameBuffer->paintBoxRel(BoxEndX-114, ChanNameY+height,   2+100+2, height2, COL_INFOBAR+7);
	g_FrameBuffer->paintBoxRel(BoxEndX-112, ChanNameY+height+2, runningPercent+2, height2-4, COL_INFOBAR+5);
	g_FrameBuffer->paintBoxRel(BoxEndX-112+runningPercent, ChanNameY+height+2, 100-runningPercent, height2-4, COL_INFOBAR+2);

	//info running
//	int start1width      = g_Fonts->infobar_info->getRenderWidth(runningStart);
	int duration1Width   = g_Fonts->infobar_info->getRenderWidth(runningDuration);
	int duration1TextPos = BoxEndX-duration1Width-10;
	height = g_Fonts->infobar_info->getHeight();
	g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR);
	g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 30,  ChanInfoY+height, duration1TextPos- (BoxStartX + ChanWidth + 40)-10, running, COL_INFOBAR);
	g_Fonts->infobar_info->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningDuration, COL_INFOBAR);

	ChanInfoY += height;

	//info next
    g_FrameBuffer->paintBox(BoxStartX + ChanWidth + 25, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

	int duration2Width   = g_Fonts->infobar_info->getRenderWidth(nextDuration);
	int duration2TextPos = BoxEndX-duration2Width-10;
	g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
	g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 30,  ChanInfoY+height, duration1TextPos- (BoxStartX + ChanWidth + 40)-10, next, COL_INFOBAR);
	g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
}

void CInfoViewer::showWarte()
{
	int height = g_Fonts->infobar_info->getHeight();
    int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
    int xStart= BoxStartX + ChanWidth + 30;

    pthread_mutex_trylock( &epg_mutex );
	g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, EPG_NotFound_Text, COL_INFOBAR);
    pthread_mutex_unlock( &epg_mutex );
}

void CInfoViewer::killTitle()
{
    pthread_mutex_lock( &epg_mutex );
    if (is_visible )
    {
    	g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX, BoxEndY );
        is_visible = false;
    }
    pthread_mutex_unlock( &epg_mutex );
}


void * CInfoViewer::LangViewerThread (void *arg)
{
	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
        pthread_mutex_lock( &InfoViewer->epg_mutex );
        pthread_cond_wait( &InfoViewer->lang_cond, &InfoViewer->epg_mutex );

        if ( ( InfoViewer->is_visible ) && ( InfoViewer->ShowInfo_Info ) )
        {
            g_RemoteControl->CopyAPIDs();
            if ( strcmp(g_RemoteControl->apid_info.name, InfoViewer->CurrentChannel.c_str() )== 0 )
            {
                InfoViewer->showButtons();
            }
        }

        pthread_mutex_unlock( &InfoViewer->epg_mutex );
    }
}

void * CInfoViewer::InfoViewerThread (void *arg)
{
    int repCount;
    string query = "";
    unsigned int    query_onid_tsid;
    bool gotEPG, requeryEPG;
    struct timespec abs_wait;
    struct timeval now;

	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
        pthread_mutex_lock( &InfoViewer->epg_mutex );
        pthread_cond_wait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex );

        if ( ( InfoViewer->is_visible ) )
        {
            gotEPG = true;
            repCount = 10;

            do
            {
                if ( !gotEPG )
                {
                    if ( repCount > 0 )
                        InfoViewer->showWarte();

//                    printf("CInfoViewer::InfoViewerThread before waiting long\n");
//                    usleep( 1000000 );

                    gettimeofday(&now, NULL);
                    TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
                    abs_wait.tv_sec += 1;

                    pthread_mutex_trylock( &InfoViewer->epg_mutex );
                    pthread_cond_timedwait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex, &abs_wait );

//                    printf("CInfoViewer::InfoViewerThread after waiting long\n");

                    repCount--;
                }

                pthread_mutex_trylock( &InfoViewer->epg_mutex );
                query = InfoViewer->CurrentChannel;
                query_onid_tsid = InfoViewer->Current_onid_tsid;
                pthread_mutex_unlock( &InfoViewer->epg_mutex );


//                printf("CInfoViewer::InfoViewerThread getEPGData for %s\n", query.c_str());

                gotEPG = InfoViewer->getEPGData(query, query_onid_tsid);

                pthread_mutex_trylock( &InfoViewer->epg_mutex );

                requeryEPG = ( ( (!gotEPG) || (query!=InfoViewer->CurrentChannel) ) &&
                               ( InfoViewer->is_visible ) );

                if (query!=InfoViewer->CurrentChannel)
                    repCount = 10;

                pthread_mutex_unlock( &InfoViewer->epg_mutex );

                if ( ( !requeryEPG) && ( InfoViewer->is_visible ) )
				{
//                    printf("CInfoViewer::InfoViewerThread success\n");
					InfoViewer->showData();
				}
                else
                {
//                    printf("CInfoViewer::InfoViewerThread unsuccessful\n");
                }


            } while ( ( requeryEPG ) && (repCount > 0) );
        }

	}
	return NULL;
}

char* copyStringto( char* from, char* to, int len)
{
	while( *from != '\n' )
	{
		if (len>2)
		{
			*to = *from;
			to++;
			len--;
		}
		from++;
	}
	*to = 0;
	from ++;
	return from;
}

bool CInfoViewer::getEPGData( string channelName, unsigned int onid_tsid )
{
	#ifdef EPG_SECTIONSD
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		bool retval = false;

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(sectionsd::portNumber);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		strcpy( running, "");
		strcpy( next, "");
		strcpy( runningStart, "");
		strcpy( nextStart, "");
		strcpy( runningDuration, "");
		strcpy( nextDuration, "");
		runningPercent = 0;

		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
			perror("Couldn't connect to server!");
			return false;
		}

        if ( ( onid_tsid != 0 ) && ( g_settings.epg_byname == 0 ) )
        {
            // query mit onid_tsid...

    		sectionsd::msgRequestHeader req;
    		req.version = 2;
    		req.command = sectionsd::currentNextInformationID;
    		req.dataLength = 4;
    		write(sock_fd,&req,sizeof(req));

            write(sock_fd, &onid_tsid, sizeof(onid_tsid));
//            char    num_evts = 2;
//            write(sock_fd, &num_evts, 1);
    		printf("query epg for onid_tsid >%x< (%s)\n", onid_tsid, channelName.c_str());

            sectionsd::msgResponseHeader resp;
        	memset(&resp, 0, sizeof(resp));

            read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

            int nBufSize = resp.dataLength;
            if(nBufSize>0)
            {
     
        		char* pData = new char[nBufSize+1] ;
                read(sock_fd, pData, nBufSize);

                unsigned long long          tmp_id;
                sectionsd::sectionsdTime*   epg_times;
                char* dp = pData;

                // current
                tmp_id = (unsigned long long)* dp;
                dp+= sizeof(tmp_id);
                epg_times = (sectionsd::sectionsdTime*) dp;
                dp+= sizeof(sectionsd::sectionsdTime);

                unsigned    dauer = epg_times->dauer/ 60;
        		sprintf((char*) &runningDuration, "%d min", dauer);

                struct      tm *pStartZeit = localtime(&epg_times->startzeit);
        		sprintf((char*) &runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                runningPercent=(unsigned)((float)(time(NULL)-epg_times->startzeit)/(float)epg_times->dauer*100.);
                strncpy(running, dp, sizeof(running));
                dp+=strlen(dp)+1;

                // next
                tmp_id = (unsigned long long)* dp;
                dp+= sizeof(tmp_id);
                epg_times = (sectionsd::sectionsdTime*) dp;
                dp+= sizeof(sectionsd::sectionsdTime);

                dauer = epg_times->dauer/ 60;
        		sprintf((char*) &nextDuration, "%d min", dauer);
                pStartZeit = localtime(&epg_times->startzeit);
        		sprintf((char*) &nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                strncpy(next, dp, sizeof(next));

        		delete[] pData;
        		retval = true;

            }
        }
        else
        {
    		sectionsd::msgRequestHeader req;
    		req.version = 2;
    		req.command = sectionsd::currentNextInformation;
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
     
        		char* pData = new char[nBufSize+1] ;
        		read(sock_fd, pData, nBufSize);
    //			printf("data: %s\n\n", pData);
        		char tmpPercent[10];
        		char tmp[20];

        		char * pos = copyStringto( pData, tmp, sizeof(tmp));
        		pos = copyStringto( pos, running, sizeof(running));
        		pos = copyStringto( pos, runningStart, sizeof(runningStart));
        		pos = copyStringto( pos, runningDuration, sizeof(runningDuration));
        		pos = copyStringto( pos, tmpPercent, sizeof(tmpPercent));
        		pos = copyStringto( pos, tmp, sizeof(tmp));
        		pos = copyStringto( pos, next, sizeof(next));
        		pos = copyStringto( pos, nextStart, sizeof(nextStart));
        		pos = copyStringto( pos, nextDuration, sizeof(nextDuration));

        		runningPercent = atoi(tmpPercent);

        		int val = atoi(runningDuration);
        		sprintf((char*) &runningDuration, "%d min", val);
        		val = atoi(nextDuration);
        		sprintf((char*) &nextDuration, "%d min", val);

        		delete[] pData;
        		retval = true;
        	}
        }

//    	printf("exit epg-get\n\n");
    	close(sock_fd);
    	return retval;
	#endif
	#ifndef EPG_SECTIONSD
		strcpy( running, "");
		strcpy( next, "");
		strcpy( runningStart, "");
		strcpy( nextStart, "");
		strcpy( runningDuration, "");
		strcpy( nextDuration, "");
		runningPercent = 0;
		return true;
	#endif
}


