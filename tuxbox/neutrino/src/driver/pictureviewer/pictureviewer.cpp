#include "config.h"
#include "pictureviewer.h"


#include <stdio.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <getopt.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

extern void fb_display(unsigned char *rgbbuff,
    int x_size, int y_size,
    int x_pan, int y_pan,
    int x_offs, int y_offs);

extern void getCurrentRes(int *x,int *y);


extern unsigned char * simple_resize(unsigned char * orgin,int ox,int oy,int dx,int dy);
extern unsigned char * color_average_resize(unsigned char * orgin,int ox,int oy,int dx,int dy);


int clear=1,delay=0,hide=1,dispinfo=1,allowstrech=0;

#ifdef FBV_SUPPORT_GIF
    extern int fh_gif_getsize(char *,int *,int*);
    extern int fh_gif_load(char *,unsigned char *,int,int);
    extern int fh_gif_id(char *);
#endif
#ifdef FBV_SUPPORT_JPEG
    extern int fh_jpeg_getsize(char *,int *,int*);
    extern int fh_jpeg_load(char *,unsigned char *,int,int);
    extern int fh_jpeg_id(char *);
#endif
#ifdef FBV_SUPPORT_PNG
    extern int fh_png_getsize(char *,int *,int*);
    extern int fh_png_load(char *,unsigned char *,int,int);
    extern int fh_png_id(char *);
#endif
#ifdef FBV_SUPPORT_BMP
    extern int fh_bmp_getsize(char *,int *,int*);
    extern int fh_bmp_load(char *,unsigned char *,int,int);
    extern int fh_bmp_id(char *);
#endif

void CPictureViewer::add_format(int (*picsize)(char *,int *,int*),int (*picread)(char *,unsigned char *,int,int), int (*id)(char*))
{
    CFormathandler *fhn;
    fhn=(CFormathandler *) malloc(sizeof(CFormathandler));
    fhn->get_size=picsize; 
	fhn->get_pic=picread; 
	fhn->id_pic=id;
    fhn->next=fh_root; 
	fh_root=fhn;
}

void CPictureViewer::init_handlers(void)
{
#ifdef FBV_SUPPORT_GIF
    add_format(fh_gif_getsize,fh_gif_load,fh_gif_id);
#endif
#ifdef FBV_SUPPORT_JPEG
    add_format(fh_jpeg_getsize,fh_jpeg_load,fh_jpeg_id);
#endif
#ifdef FBV_SUPPORT_PNG
    add_format(fh_png_getsize,fh_png_load,fh_png_id);
#endif
#ifdef FBV_SUPPORT_BMP
    add_format(fh_bmp_getsize,fh_bmp_load,fh_bmp_id);
#endif
}

CPictureViewer::CFormathandler * CPictureViewer::fh_getsize(char *name,int *x,int *y)
{
   dbout("fh_getsize {\n"); 
	CFormathandler *fh;
    for(fh=fh_root;fh!=NULL;fh=fh->next)
    {
	if(fh->id_pic(name))
	    if(fh->get_size(name,x,y)==FH_ERROR_OK) return(fh);
    }
	 dbout("fh_getsize }\n"); 
    return(NULL);
}

int CPictureViewer::show_image(char *name)
{
   dbout("show_image {\n"); 
	int x,y,xs,ys,xpos,ypos,xdelta,ydelta,eol,xstep,ystep,imx,imy;
    unsigned char *buffer;
    CFormathandler *fh;
    eol=1;
    if((fh=fh_getsize(name,&x,&y)))
    {
		buffer=(unsigned char *) malloc(x*y*3);
		if(fh->get_pic(name,buffer,x,y)==FH_ERROR_OK)
		{
			getCurrentRes(&xs,&ys);
			if((x>xs || y>ys) && allowstrech)
			{
				if( (y*xs/x) <= ys)
				{
					imx=xs;
					imy=y*xs/x;
				}
				else
				{
					imx=x*ys/y;
					imy=ys;
				}
				if(allowstrech==1)
					buffer=simple_resize(buffer,x,y,imx,imy);
				else
					buffer=color_average_resize(buffer,x,y,imx,imy);
				x=imx; y=imy;
			}
			if(x<xs) xpos=(xs-x)/2; else xpos=0;
			if(y<ys) ypos=(ys-y)/2; else ypos=0;
			xdelta=0; ydelta=0;

			xstep=min(max(x/20,1),xs);
			ystep=min(max(y/20,1),ys);
			fb_display(buffer,x,y,xdelta,ydelta,xpos,ypos);

/*
			for(eol=-1,rfrsh=1;eol==-1 ;)
			{
				if(rfrsh) fb_display(buffer,x,y,xdelta,ydelta,xpos,ypos);
				rfrsh=0;
				if(!delay)
				{
					c=getchar();
					switch(c)
					{
						case 'a': case 'D':
								xdelta-=xstep;
							if(xdelta<0) xdelta=0;
							rfrsh=1;
							break;
						case 'd': case 'C':
							if(xpos) break;
							xdelta+=xstep;
							if(xdelta>(x-xs)) xdelta=x-xs;
							rfrsh=1;
							break;
						case 'w': case 'A':
							ydelta-=ystep;
							if(ydelta<0) ydelta=0;
							rfrsh=1;
							break;
						case 'x': case 'B':
							if(ypos) break;
							ydelta+=ystep;
							if(ydelta>(y-ys)) ydelta=y-ys;
							rfrsh=1;
							break;
						case ' ': case 10: eol=1; break;
						case 'r': rfrsh=1; break;
						case '.': case '>': eol=NEXT_IMG; break;
						case ',': case '<': case 127: case 255: eol=PREV_IMG; break;
						case 'q': eol=0; break;
					}
				}
				else
				{
					if(imm_getchar(delay / 10,delay % 10)=='q') eol=0; else eol=1;
					break;
				}
			}
*/
		}
		else
			printf("Unable to read file !\n");
		free(buffer);
	}
	else
		printf("Unable to read file or format not recognized!\n");
	
   dbout("show_image }\n"); 
	return(eol);
}


bool CPictureViewer::ShowImage(std::string filename)
{
    int r;
    init_handlers();																       
    r=show_image((char *)filename.c_str());
    return true;
}
