#include "streaminfo.h"


CStreamInfo::CStreamInfo(FontsDef *Fonts)
{
	fonts = Fonts;
	width = 250;
	height = 150;
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
	fonts->menu_title->RenderString(x+10,y+23, width,"Streaminfo", COL_MENUHEAD);
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
	sprintf((char*) buf, "Bitrate: %d", bitInfo[4]);
	fonts->menu->RenderString(x+10,y+80, width, buf, COL_MENUCONTENT);
	sprintf((char*) buf, "Framerate: %d", bitInfo[3]);
	fonts->menu->RenderString(x+10,y+100, width, buf, COL_MENUCONTENT);
	sprintf((char*) buf, "Audiotype: %d", bitInfo[6]);
	fonts->menu->RenderString(x+10,y+120, width, buf, COL_MENUCONTENT);
}

