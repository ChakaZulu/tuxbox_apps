#include "streaminfo.h"


CStreamInfo::CStreamInfo(FontsDef *Fonts)
{
	fonts = Fonts;
	width = 300;
	height = 155;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}


int CStreamInfo::exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide(frameBuffer);
	}
	paint(frameBuffer);

	rcInput->getKey(130); 

	hide(frameBuffer);
	return CMenuTarget::RETURN_REPAINT;
}

void CStreamInfo::hide(CFrameBuffer* frameBuffer)
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CStreamInfo::paint(CFrameBuffer* frameBuffer)
{

	frameBuffer->paintBoxRel(x,y, width,30, COL_MENUHEAD);
	fonts->menu_title->RenderString(x+10,y+30, width,"Streaminfo", COL_MENUHEAD);
	frameBuffer->paintBoxRel(x,y+30, width,height-30, COL_MENUCONTENT);

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
	fonts->menu->RenderString(x+10,y+60, width, buf, COL_MENUCONTENT);

	sprintf((char*) buf, "Bitrate: %d bit/sec", bitInfo[4]*50);
	fonts->menu->RenderString(x+10,y+80, width, buf, COL_MENUCONTENT);


	switch ( bitInfo[2] )
	{
		case 2: strcpy(buf, "Aspect Ratio: 4:3"); break;
		case 3: strcpy(buf, "Aspect Ratio: 16:9"); break;
		case 4: strcpy(buf, "Aspect Ratio: 2.21:1"); break;
		default: strcpy(buf, "Aspect Ratio: unknown");
	}
	fonts->menu->RenderString(x+10,y+100, width, buf, COL_MENUCONTENT);


	switch ( bitInfo[3] )
	{
		case 3: strcpy(buf, "Framerate: 25fps"); break;
		case 6: strcpy(buf, "Framerate: 50fps"); break;
		default: strcpy(buf, "Framerate: unknown");
	}
	fonts->menu->RenderString(x+10,y+120, width, buf, COL_MENUCONTENT);


	switch ( bitInfo[6] )
	{
		case 1: strcpy(buf, "Audiotype:  single channel"); break;
		case 2: strcpy(buf, "Audiotype:  dual channel"); break;
		case 3: strcpy(buf, "Audiotype:  joint stereo"); break;
		case 4: strcpy(buf, "Audiotype:  stereo"); break;
		default: strcpy(buf, "Audiotype: unknown");
	}
	fonts->menu->RenderString(x+10,y+140, width, buf, COL_MENUCONTENT);
}

