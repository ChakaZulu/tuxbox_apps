#include "infoviewer.h"




CInfoViewer::CInfoViewer()
{
        intTimer = 0;
        intShowDuration = 15; //7,5 sec
        frameBuffer = NULL;
        fonts = NULL;
		InfoHeightY = 170;

		strcpy( running, "");
		strcpy( next, "");
		strcpy( runningStart, "");
		strcpy( nextStart, "");
		strcpy( runningDuration, "");
		strcpy( nextDuration, "");
		runningPercent = 0;
		CurrentChannel = "";
		epgReady = true;
}

void CInfoViewer::start(CFrameBuffer *FrameBuffer, FontsDef *Fonts, SNeutrinoSettings *Settings )
{
        frameBuffer = FrameBuffer;
        fonts = Fonts;
		settings = Settings;
        if (pthread_create (&thrViewer, NULL, InfoViewerThread, (void *) this) != 0 )
        {
                perror("create failed\n");
        }
}

void CInfoViewer::setDuration( int Duration )
{
        intShowDuration = Duration;
}

void CInfoViewer::showTitle( int ChanNum, string Channel )
{
	if (CurrentChannel==Channel)
	{
		intTimer = intShowDuration;
		return;
	}
	CurrentChannel = Channel;

	int ChanWidth  = 100;
	int ChanHeight = 70;

	int ChanNameX = settings->screen_StartX + ChanWidth + 30;
	int ChanNameY = settings->screen_EndY-InfoHeightY + (ChanHeight>>1);

	int ChanInfoX = settings->screen_StartX + 20+ (ChanWidth >>1);
	int ChanInfoY = settings->screen_EndY-InfoHeightY + ChanHeight+10;

	BoxEndX = settings->screen_EndX-20;
	BoxEndY = settings->screen_EndY-20;
	BoxStartX = settings->screen_StartX+20;
	BoxStartY = settings->screen_EndY-InfoHeightY;


	//number box
	frameBuffer->paintBoxRel(settings->screen_StartX+30, settings->screen_EndY-InfoHeightY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW);
	frameBuffer->paintBoxRel(settings->screen_StartX+20, settings->screen_EndY-InfoHeightY, ChanWidth, ChanHeight, COL_INFOBAR);

	//infobox
	frameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndY, COL_INFOBAR);
	frameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndY, COL_INFOBAR);

	//channel number
	char strChanNum[10];
	sprintf( (char*) strChanNum, "%d", ChanNum);
	int ChanNumXPos = settings->screen_StartX+20+ ((ChanWidth - fonts->infobar_number->getRenderWidth(strChanNum))>>1);
	fonts->infobar_number->RenderString(ChanNumXPos, settings->screen_EndY-InfoHeightY+ChanHeight-18, ChanWidth+30, strChanNum, COL_INFOBAR);

	//channel name
	fonts->infobar_channame->RenderString(ChanNameX+15, ChanNameY+35, BoxEndX-ChanNameX-135, Channel.c_str(), COL_INFOBAR);

	//epg-data?
	intTimer = intShowDuration;
	epgReady = false;
/*
	if (getEPGData( Channel.c_str() ))
		showData();
*/
}

void CInfoViewer::showData()
{
	int ChanWidth  = 100;
	int ChanHeight = 70;

	int ChanNameY = settings->screen_EndY-InfoHeightY + (ChanHeight>>1);

	int ChanInfoX = settings->screen_StartX + 20+ (ChanWidth >>1);
	int ChanInfoY = settings->screen_EndY-InfoHeightY + ChanHeight+10;
	
	//proz.
	frameBuffer->paintBoxRel(BoxEndX-114, ChanNameY+10, 104, 24, COL_INFOBAR+7);
        frameBuffer->paintBoxRel(BoxEndX-112, ChanNameY+12, runningPercent+2, 20, COL_INFOBAR+5);
	frameBuffer->paintBoxRel(BoxEndX-112+runningPercent, ChanNameY+12, 100-runningPercent, 20, COL_INFOBAR+2);

	//info running
	int duration1Width = fonts->infobar_info->getRenderWidth(runningDuration); 
	int duration1TextPos = BoxEndX-duration1Width-10;
	fonts->infobar_info->RenderString(ChanInfoX+15, ChanInfoY+30, 100, runningStart, COL_INFOBAR);
	fonts->infobar_info->RenderString(ChanInfoX+100, ChanInfoY+30, duration1TextPos-(ChanInfoX+100)-15, running, COL_INFOBAR);
	fonts->infobar_info->RenderString(duration1TextPos, ChanInfoY+30, 100, runningDuration, COL_INFOBAR);

	//info next
	int duration2Width = fonts->infobar_info->getRenderWidth(nextDuration); 
	int duration2TextPos = BoxEndX-duration2Width-10;
	fonts->infobar_info->RenderString(ChanInfoX+15, ChanInfoY+55, 100, nextStart, COL_INFOBAR);
	fonts->infobar_info->RenderString(ChanInfoX+100, ChanInfoY+55, duration2TextPos-(ChanInfoX+100)-15, next, COL_INFOBAR);
	fonts->infobar_info->RenderString(duration2TextPos, ChanInfoY+55, 100, nextDuration, COL_INFOBAR);
}

void CInfoViewer::killTitle()
{
	intTimer = 0;
	frameBuffer->paintBox(BoxStartX, BoxStartY, BoxEndX, BoxEndY, COL_BACKGROUND);
}

void * CInfoViewer::InfoViewerThread (void *arg)
{
	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
		usleep(500000);
		if(InfoViewer->intTimer>0)
		{
			InfoViewer->intTimer--;
			if(InfoViewer->intTimer==0)
			{
				InfoViewer->killTitle();
			}
			else
			{
				//epg
				if((!InfoViewer->epgReady) && (InfoViewer->intTimer&1))
				{
					string query = InfoViewer->CurrentChannel;
					if( (InfoViewer->getEPGData(query)) && (query==InfoViewer->CurrentChannel))
					{
						InfoViewer->epgReady = true;
						InfoViewer->showData();
					}
				}
			}
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

bool CInfoViewer::getEPGData( string channelName )
{
	#ifdef EPG_SECTIONSD
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		bool retval = false;

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1600);
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

		msgSectionsdRequestHeader req;
		req.version = 2;
		req.command = currentNextInformation;
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

		msgSectionsdResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(msgSectionsdResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
			char* pData = new char[nBufSize+1] ;
			read(sock_fd, pData, nBufSize);
			printf("data: %s\n\n", pData);
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

		printf("exit epg-get\n\n");
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
