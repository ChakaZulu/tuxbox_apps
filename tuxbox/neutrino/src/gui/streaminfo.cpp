#include "streaminfo.h"
#include "../global.h"

CStreamInfo::CStreamInfo()
{
	width = 300;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}


int CStreamInfo::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int key = g_RCInput->getKey(130);

    if ( (key==CRCInput::RC_spkr) ||
	     (key==CRCInput::RC_plus) ||
         (key==CRCInput::RC_minus) )
    {
        g_RCInput->addKey2Buffer(key);
    }

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
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, "Streaminfo", COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight;
	
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
	sprintf((char*) buf, "Resolution: %dx%d", bitInfo[0], bitInfo[1] );
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;

	sprintf((char*) buf, "Bitrate: %d bit/sec", bitInfo[4]*50);
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[2] )
	{
		case 2: strcpy(buf, "Aspect Ratio: 4:3"); break;
		case 3: strcpy(buf, "Aspect Ratio: 16:9"); break;
		case 4: strcpy(buf, "Aspect Ratio: 2.21:1"); break;
		default: strcpy(buf, "Aspect Ratio: unknown");
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[3] )
	{
		case 3: strcpy(buf, "Framerate: 25fps"); break;
		case 6: strcpy(buf, "Framerate: 50fps"); break;
		default: strcpy(buf, "Framerate: unknown");
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;


	switch ( bitInfo[6] )
	{
		case 1: strcpy(buf, "Audiotype:  single channel"); break;
		case 2: strcpy(buf, "Audiotype:  dual channel"); break;
		case 3: strcpy(buf, "Audiotype:  joint stereo"); break;
		case 4: strcpy(buf, "Audiotype:  stereo"); break;
		default: strcpy(buf, "Audiotype: unknown");
	}
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);

	ypos+= mheight;
}



