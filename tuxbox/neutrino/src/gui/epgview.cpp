#include "epgdata.h"



static char* ocopyStringto(const char* from, char* to, int len)
{
	while(*from!='\xff' && from<from+len)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

CEpgData::CEpgData()
{
}

void CEpgData::start(CFrameBuffer *FrameBuffer, FontsDef *Fonts, CRCInput* RcInput, SNeutrinoSettings *Settings )
{
	frameBuffer = FrameBuffer;
	fonts = Fonts;
	rcInput = RcInput;
	settings = Settings;

	ox = 500;
	oy = 290;

	sx = (((settings->screen_EndX-settings->screen_StartX)-ox) / 2) + settings->screen_StartX;
	sy = (((settings->screen_EndY-settings->screen_StartY)-oy) / 2) + settings->screen_StartY;
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
	int		aktWidth = 0;

	//printf("orginaltext:\n%s\n\n", text);
	while(*text!=0)
	{
		if ((*text==' ') || (*text=='\n'))
		{
			//check the wordwidth - add to this line if size ok
			aktWord += ' ';
			int aktWordWidth = fonts->epg_info2->getRenderWidth(aktWord.c_str());
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
			if(*text=='\n')
			{	//enter-handler
				//printf("enter-");
				addTextToArray( aktLine );
				aktLine = "";
				aktWidth= 0;
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

void CEpgData::showText( int startPos)
{
	int ypos = sy+70;
	int textCount = epgText.size();
	frameBuffer->paintBoxRel(sx, sy+45, ox, oy-65, COL_MENUCONTENT);
	if(startPos==0)
	{
		fonts->epg_info1->RenderString(sx+10, ypos, ox-15, epgData.info1, COL_MENUCONTENT);
		ypos += 25;
	}
	while (ypos<oy+sy-25)
	{
		string text="";
		if(startPos<textCount)
		{
			text = epgText[startPos];
		}
		fonts->epg_info2->RenderString(sx+10, ypos, ox, text.c_str(), COL_MENUCONTENT);
		ypos +=20;	
		startPos++;
	}
}

void CEpgData::show( string channelName  )
{
	frameBuffer->paintBoxRel(settings->screen_StartX, settings->screen_StartY, 50, 30, COL_INFOBAR);
	fonts->epg_date->RenderString(settings->screen_StartX+10, settings->screen_StartY+20, 40, "-@-", COL_INFOBAR);

	GetEPGData( channelName );
	frameBuffer->paintBoxRel(settings->screen_StartX, settings->screen_StartY, 50, 30, COL_BACKGROUND);

	if(strlen(epgData.title)==0)
	{
		//no epg-data found :(
		char *text = "no epg found";
		int oy = 30;
		int ox = fonts->epg_info2->getRenderWidth(text)+30;
		int sx = (((settings->screen_EndX-settings->screen_StartX)-ox) / 2) + settings->screen_StartX;
		int sy = (((settings->screen_EndY-settings->screen_StartY)-oy) / 2) + settings->screen_StartY;
		frameBuffer->paintBoxRel(sx, sy, ox, 30, COL_MENUHEAD);
		fonts->epg_info2->RenderString(sx+15, sy+20, ox-15, "no epg found", COL_MENUHEAD);
		rcInput->getKey(20); 
		frameBuffer->paintBoxRel(sx, sy, ox, 30, COL_BACKGROUND);
		return;
	}


	//scan epg-data - sort to list
	if(strlen(epgData.info2)!=0)
	{
		processTextToArray( epgData.info2 );
	}

	//show the epg
	frameBuffer->paintBoxRel(sx, sy, ox, 45, COL_MENUHEAD);
	frameBuffer->paintBoxRel(sx, sy+oy-20, ox, 20, COL_MENUHEAD);

	fonts->epg_title->RenderString(sx+10, sy+35, ox-15, epgData.title, COL_MENUHEAD);

	int showPos = 0;
	int textCount = epgText.size();
	showText(showPos);

	//show date-time....
	char fromto[40];
	strcpy( fromto, epgData.start );
	strcat( fromto, " - " );
	strcat( fromto, epgData.end);
	fonts->epg_date->RenderString(sx+10,  sy+oy-4, 380, fromto, COL_MENUHEAD);
	fonts->epg_date->RenderString(sx+ox-110,  sy+oy-4, 110, epgData.date, COL_MENUHEAD);

	//show progessbar
	int progress = atoi(epgData.done);
	printf("prog: %d\n", progress);
	frameBuffer->paintBoxRel(sx+200, sy+oy-15, 102, 9, COL_MENUHEAD+5);
	frameBuffer->paintBoxRel(sx+201, sy+oy-14, 100, 7, COL_MENUHEAD);
	frameBuffer->paintBoxRel(sx+201, sy+oy-14, progress, 7, COL_MENUHEAD+5);



	bool loop=true;
	int scrollCount;
	while(loop)
	{
		int key = rcInput->getKey(40); 
		if(showPos==0)
		{	//titleinfo exists
			scrollCount = 9;
		}
		else
			scrollCount = 10;

		if (key==CRCInput::RC_down)
		{
			if(showPos+scrollCount<textCount)
			{
				showPos += scrollCount;
				showText(showPos);
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
			if((showPos==0) && (scrollCount==10))
			{
				toShow = true;
			}
			if (toShow)
				showText(showPos);
		}
		else if ((key==CRCInput::RC_ok) || (key==CRCInput::RC_help))
		{
			loop = false;
		}
	}
	hide();
}

void CEpgData::hide()
{
	frameBuffer->paintBoxRel (sx, sy, ox+10, oy+10, 255);
}

void CEpgData::GetEPGData( string channelName )
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
	servaddr.sin_port=htons(1600);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to server!");
		return;
	}

	#ifdef EPG_SECTIONSD
		//use new sectionsd-daemon
		msgSectionsdRequestHeader req;
		req.version = 2;
		req.command = actualEPGchannelName;
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
			printf("neutrino nBufsize: %d\n", nBufSize);
			char* pData = new char[nBufSize] ;
			read(sock_fd, pData, nBufSize);
			printf("neutrino epgdata: \n%s\n", pData);

			char tmp[20];
			char *pos = ocopyStringto( pData, tmp, sizeof(tmp));
			printf("id: %s\n", tmp);
			pos = ocopyStringto( pos, epgData.title, sizeof(epgData.title));
			printf("title: %s\n", epgData.title);
			pos = ocopyStringto( pos, epgData.info1, sizeof(epgData.info1) );
			pos = ocopyStringto( pos, epgData.info2, sizeof(epgData.info2));
			pos = ocopyStringto( pos, epgData.date, sizeof(epgData.date));
			pos = ocopyStringto( pos, epgData.start, sizeof(epgData.start));
			pos = ocopyStringto( pos, epgData.end, sizeof(epgData.end));
			pos = ocopyStringto( pos, epgData.done, sizeof(epgData.done));

			delete[] pData;
			printf("copied\n");
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

		printf("neutrino epgdata: %i: %s\n", rep.version, rep.sizeOfBuffer);

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
