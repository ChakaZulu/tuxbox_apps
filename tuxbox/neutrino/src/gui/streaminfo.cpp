/*
$Id: streaminfo.cpp,v 1.9 2001/10/09 21:48:37 McClean Exp $

Module StreamInfo

History:
 $Log: streaminfo.cpp,v $
 Revision 1.9  2001/10/09 21:48:37  McClean
 ucode-check

 Revision 1.8  2001/09/23 21:34:07  rasc
 - LIFObuffer Module, pushbackKey fuer RCInput,
 - In einige Helper und widget-Module eingebracht
   ==> harmonischeres Menuehandling
 - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)


*/


#include "streaminfo.h"
#include "../global.h"

CStreamInfo::CStreamInfo()
{
	width = 300;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+6*mheight;
	x=((720-width) >> 1) -20;
	y=(400-height)>>1;
}


int CStreamInfo::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

//	int key = g_RCInput->getKey(130);

	// -- just eat key and return

	g_RCInput->getKey(130);

//    if ( (key==CRCInput::RC_spkr) ||
//	     (key==CRCInput::RC_plus) ||
//         (key==CRCInput::RC_minus) )
//    {
//        g_RCInput->pushbackKey(key);
//    }

	hide();
	return CMenuTarget::RETURN_REPAINT;
}

void CStreamInfo::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CStreamInfo::paint()
{
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("streaminfo.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);

	
	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return;
	}

	int bitInfo[10];

	char *key,*tmpptr,buf[100];
	int value, pos=0;
	fgets(buf,29,fd);//dummy
	while(!feof(fd)) 
	{
		if(fgets(buf,29,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			key=strsep(&tmpptr,":");
			for(;tmpptr[0]==' ';tmpptr++);
			value=atoi(tmpptr);
			//printf("%s: %d\n",key,value);
			bitInfo[pos]= value;
			pos++;
		}
	}
	fclose(fd);

	
	//paint msg...
	sprintf((char*) buf, "%s: %dx%d", g_Locale->getText("streaminfo.resolution").c_str(), bitInfo[0], bitInfo[1] );
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;

	sprintf((char*) buf, "%s: %d bit/sec", g_Locale->getText("streaminfo.bitrate").c_str(), bitInfo[4]*50);
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[2] )
	{
		case 2: sprintf((char*) buf, "%s: 4:3", g_Locale->getText("streaminfo.aratio").c_str() ); break;
		case 3: sprintf((char*) buf, "%s: 16:9", g_Locale->getText("streaminfo.aratio").c_str()); break;
		case 4: sprintf((char*) buf, "%s: 2.21:1", g_Locale->getText("streaminfo.aratio").c_str()); break;
		default: sprintf((char*) buf, "%s", g_Locale->getText("streaminfo.aratio_unknown").c_str());
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[3] )
	{
		case 3:  sprintf((char*) buf, "%s: 25fps", g_Locale->getText("streaminfo.framerate").c_str()); break;
		case 6:  sprintf((char*) buf, "%s: 50fps", g_Locale->getText("streaminfo.framerate").c_str()); break;
		default:  sprintf((char*) buf, "%s", g_Locale->getText("streaminfo.framerate_unknown").c_str());
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[6] )
	{
		case 1: sprintf((char*) buf, "%s: single channel", g_Locale->getText("streaminfo.audiotype").c_str()); break;
		case 2: sprintf((char*) buf, "%s: dual channel", g_Locale->getText("streaminfo.audiotype").c_str()); break;
		case 3: sprintf((char*) buf, "%s: joint stereo", g_Locale->getText("streaminfo.audiotype").c_str()); break;
		case 4: sprintf((char*) buf, "%s: stereo", g_Locale->getText("streaminfo.audiotype").c_str()); break;
		default:  sprintf((char*) buf, "%s", g_Locale->getText("streaminfo.audiotype_unknown").c_str()); 
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;
}
