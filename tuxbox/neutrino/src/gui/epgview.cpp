//
// $Id: epgview.cpp,v 1.6 2001/08/20 13:07:10 tw-74 Exp $
//
// $Log: epgview.cpp,v $
// Revision 1.6  2001/08/20 13:07:10  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.5  2001/08/16 00:19:44  fnbrd
// Removed debug output.
//
//

#include "epgdata.h"


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

void CEpgData::start(CFrameBuffer *FrameBuffer, FontsDef *Fonts, CRCInput* RcInput, SNeutrinoSettings *Settings )
{
	frameBuffer = FrameBuffer;
	fonts = Fonts;
	rcInput = RcInput;
	settings = Settings;

	ox = 500;
	oy = 290;

	topheight=fonts->epg_title->getHeight();
	topboxheight=topheight+10;
	botheight=fonts->epg_date->getHeight();
	botboxheight=botheight+6;
	medlineheight=fonts->epg_info->getHeight();
	medlinecount=(oy-topboxheight-botboxheight)/medlineheight;

	oy=topboxheight+botboxheight+medlinecount*medlineheight; // recalculate

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
	int	aktWidth = 0;

	//printf("orginaltext:\n%s\n\n", text);
	while(*text!=0)
	{
		if ((*text==' ') || (*text=='\n'))
		{
			//check the wordwidth - add to this line if size ok
			aktWord += ' ';
			int aktWordWidth = fonts->epg_info->getRenderWidth(aktWord.c_str());
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

void CEpgData::showText( int startPos, int ypos )
{
	int textCount = epgText.size();
	int y=ypos;
	int linecount=medlinecount;
	string t;
	frameBuffer->paintBoxRel(sx, y, ox, linecount*medlineheight, COL_MENUCONTENT);
	if(startPos==0){
		t=epgData.info1;
		fonts->epg_info->RenderString(sx+10,y+medlineheight,ox-15,t.c_str(),COL_MENUCONTENT);
		y+=medlineheight;
		linecount--;
	}
	for(int i=startPos; i<textCount && i<startPos+linecount; i++,y+=medlineheight)
	{
		t=epgText[i];
		fonts->epg_info->RenderString(sx+10, y+medlineheight, ox-15, t.c_str(), COL_MENUCONTENT);
	}
}

void CEpgData::show( string channelName )
{
	int height;
	height=fonts->epg_date->getHeight();
	frameBuffer->paintBoxRel(settings->screen_StartX, settings->screen_StartY, 50, height+5, COL_INFOBAR);
	fonts->epg_date->RenderString(settings->screen_StartX+10, settings->screen_StartY+height, 40, "-@-", COL_INFOBAR);

	GetEPGData( channelName );
	frameBuffer->paintBoxRel(settings->screen_StartX, settings->screen_StartY, 50, height+5, COL_BACKGROUND);

	if(strlen(epgData.title)==0)
	{
		//no epg-data found :(
		char *text = "no epg found";
		int oy = 30;
		int ox = fonts->epg_info->getRenderWidth(text)+30;
		int sx = (((settings->screen_EndX-settings->screen_StartX)-ox) / 2) + settings->screen_StartX;
		int sy = (((settings->screen_EndY-settings->screen_StartY)-oy) / 2) + settings->screen_StartY;
		height=fonts->epg_info->getHeight();
		frameBuffer->paintBoxRel(sx, sy, ox, height+10, COL_MENUHEAD);
		fonts->epg_info->RenderString(sx+15, sy+height+5, ox-15, text, COL_MENUHEAD);
		rcInput->getKey(20); 
		frameBuffer->paintBoxRel(sx, sy, ox, height+10, COL_BACKGROUND);
		return;
	}

	//scan epg-data - sort to list
	if(strlen(epgData.info2)!=0)
	{
		processTextToArray( epgData.info2 );
	}

	//show the epg
	frameBuffer->paintBoxRel(sx, sy, ox, topboxheight, COL_MENUHEAD);
	fonts->epg_title->RenderString(sx+10, sy+topheight+5, ox-15, epgData.title, COL_MENUHEAD);

	//show date-time....
	frameBuffer->paintBoxRel(sx, sy+oy-botboxheight, ox, botboxheight, COL_MENUHEAD);
	char fromto[40];
	int widthl,widthr;
	strcpy(fromto,epgData.start); strcat(fromto," - "); strcat(fromto,epgData.end);
	widthl=fonts->epg_date->getRenderWidth(fromto);
	fonts->epg_date->RenderString(sx+10,  sy+oy-3, widthl, fromto, COL_MENUHEAD);
	widthr=fonts->epg_date->getRenderWidth(epgData.date);
	fonts->epg_date->RenderString(sx+ox-10-widthr,  sy+oy-3, widthr, epgData.date, COL_MENUHEAD);

	int showPos = 0;
	int textCount = epgText.size();
	int textypos = sy+topboxheight;
	showText(showPos,textypos);

	//show progressbar
	int progress = atoi(epgData.done);
	printf("prog: %d\n", progress);
	int pbx = sx + 10 + widthl + 10 + ((ox-104-widthr-widthl-10-10-20)>>1);
	frameBuffer->paintBoxRel(pbx, sy+oy-height, 104, height-6, COL_MENUHEAD+7);
	frameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, 100, height-10, COL_MENUHEAD+2);
	frameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, progress, height-10, COL_MENUHEAD+5);

	bool loop=true;
	int scrollCount;
	while(loop)
	{
		int key = rcInput->getKey(40); 

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
	servaddr.sin_port=htons(sectionsd::portNumber);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to server!");
		return;
	}

	#ifdef EPG_SECTIONSD
		//use new sectionsd-daemon
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
