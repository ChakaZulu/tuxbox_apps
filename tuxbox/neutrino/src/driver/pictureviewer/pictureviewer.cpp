#include "config.h"
#include "pictureviewer.h"
#include "fb_display.h"
#include "driver/framebuffer.h"


#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <getopt.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

extern unsigned char * simple_resize(unsigned char * orgin,int ox,int oy,int dx,int dy);
extern unsigned char * color_average_resize(unsigned char * orgin,int ox,int oy,int dx,int dy);


#ifdef FBV_SUPPORT_GIF
    extern int fh_gif_getsize(const char *,int *,int*,int);
    extern int fh_gif_load(const char *,unsigned char *,int,int);
    extern int fh_gif_id(const char *);
#endif
#ifdef FBV_SUPPORT_JPEG
    extern int fh_jpeg_getsize(const char *,int *,int*,int);
    extern int fh_jpeg_load(const char *,unsigned char *,int,int);
    extern int fh_jpeg_id(const char *);
#endif
#ifdef FBV_SUPPORT_PNG
    extern int fh_png_getsize(const char *,int *,int*,int);
    extern int fh_png_load(const char *,unsigned char *,int,int);
    extern int fh_png_id(const char *);
#endif
#ifdef FBV_SUPPORT_BMP
    extern int fh_bmp_getsize(const char *,int *,int*,int);
    extern int fh_bmp_load(const char *,unsigned char *,int,int);
    extern int fh_bmp_id(const char *);
#endif

void CPictureViewer::add_format(int (*picsize)(const char *,int *,int*,int ),int (*picread)(const char *,unsigned char *,int,int), int (*id)(const char*))
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

CPictureViewer::CFormathandler * CPictureViewer::fh_getsize(const char *name,int *x,int *y, int width_wanted)
{
	CFormathandler *fh;
    for(fh=fh_root;fh!=NULL;fh=fh->next)
    {
	if(fh->id_pic(name))
	    if(fh->get_size(name,x,y,width_wanted)==FH_ERROR_OK) return(fh);
    }
    return(NULL);
}

bool CPictureViewer::DecodeImage(std::string name, int startx, int endx, int starty, int endy, bool showBusySign)
{
//   dbout("DecodeImage {\n"); 
	int x,y,xs,ys,imx,imy;
	getCurrentRes(&xs,&ys);
	
	// Show red block for "next ready" in view state
	if(showBusySign)
		showBusy(startx+3,starty+3,10,0xff,00,00);
   
	CFormathandler *fh;
   if((fh=fh_getsize(name.c_str(),&x,&y,endx-startx)))
   {
      if(m_Pic_Buffer!=NULL)
      {
         if(x!=m_Pic_X || y!=m_Pic_Y)
         {
            free(m_Pic_Buffer);
            m_Pic_Buffer=(unsigned char *) malloc(x*y*3);
         }
      }
      else
         m_Pic_Buffer=(unsigned char *) malloc(x*y*3);
		
//      dbout("---Decoding Start(%d/%d)\n",x,y);
		if(fh->get_pic(name.c_str(),m_Pic_Buffer,x,y)==FH_ERROR_OK)
		{
//			dbout("---Decoding Done\n");
			if((x>(endx-startx) || y>(endy-starty)) && m_scaling!=NONE)
			{
				double aspect_ratio_correction = m_aspect / ((double)xs/ys); 
				if( (aspect_ratio_correction*y*(endx-startx)/x) <= (endy-starty))
				{
					imx=(endx-startx);
					imy=(int)(aspect_ratio_correction*y*(endx-startx)/x);
				}
				else
				{
					imx=(int)((1.0/aspect_ratio_correction)*x*(endy-starty)/y);
					imy=(endy-starty);
				}
				if(m_scaling==SIMPLE)
					m_Pic_Buffer=simple_resize(m_Pic_Buffer,x,y,imx,imy);
				else
					m_Pic_Buffer=color_average_resize(m_Pic_Buffer,x,y,imx,imy);
				x=imx; y=imy;
			}
         m_Pic_Name = name;
         m_Pic_X=x;
         m_Pic_Y=y;
			if(x<xs) 
            m_Pic_XPos=(endx-startx-x)/2+startx; 
         else 
            m_Pic_XPos=startx;
			if(y<ys) 
            m_Pic_YPos=(endy-starty-y)/2+starty; 
         else 
            m_Pic_YPos=0;
			if(x > xs)
            m_Pic_XPan=(x-xs) / 2;
         else
            m_Pic_XPan=0;
			if(y > ys)
            m_Pic_YPan=(y-ys) / 2;
         else
            m_Pic_YPan=0;
		}
		else
      {
			printf("Unable to read file !\n");
         free(m_Pic_Buffer);
			m_Pic_Buffer=(unsigned char *) malloc(xs*ys*3);
			memset(m_Pic_Buffer, 0 , xs*ys*3);
         m_Pic_X=xs;
         m_Pic_Y=ys;
			m_Pic_XPos=0;
			m_Pic_YPos=0;
			m_Pic_XPan=0;
			m_Pic_YPan=0;
      }
	}
	else
   {
      if(m_Pic_Buffer!=NULL)
      {
         free(m_Pic_Buffer);
      }
		m_Pic_Buffer=(unsigned char *) malloc(xs*ys*3);
		memset(m_Pic_Buffer, 0 , xs*ys*3);
		m_Pic_X=xs;
		m_Pic_Y=ys;
		m_Pic_XPos=0;
		m_Pic_YPos=0;
		m_Pic_XPan=0;
		m_Pic_YPan=0;
		printf("Unable to read file or format not recognized!\n");
   }
	hideBusy();
//   dbout("DecodeImage }\n"); 
	return(m_Pic_Buffer!=NULL);
}


bool CPictureViewer::ShowImage(std::string filename, int startx, int endx, int starty, int endy)
{
//	dbout("Show Image {\n");
   if(filename!=m_Pic_Name)
   {
      // Picture not yet decoded
      DecodeImage(filename,startx,endx,starty,endy,false);
   }
   DisplayImage();
//	dbout("Show Image }\n");
   return true;
}
bool CPictureViewer::DisplayImage()
{
   if(m_Pic_Buffer != NULL)
      fb_display(m_Pic_Buffer, m_Pic_X, m_Pic_Y, m_Pic_XPan, m_Pic_YPan,
                 m_Pic_XPos, m_Pic_YPos);
	return true;
}

CPictureViewer::CPictureViewer()
{
	fh_root=NULL;
	m_scaling=NONE;
	m_aspect=4.0 / 3;
   m_Pic_Name="";
   m_Pic_Buffer=NULL;
   m_Pic_X=0;
   m_Pic_Y=0;
   m_Pic_XPos=0;
   m_Pic_YPos=0;
   m_Pic_XPan=0;
   m_Pic_YPan=0;
	
	init_handlers();																     
}

void CPictureViewer::showBusy(int sx, int sy, int width, char r, char g, char b)
{
//	dbout("Show Busy{\n");
	unsigned char rgb_buffer[3];
	unsigned char* fb_buffer;
	unsigned char* busy_buffer_wrk;
	int cpp;
	struct fb_var_screeninfo *var;
	var = CFrameBuffer::getInstance()->getScreenInfo();
	
	rgb_buffer[0]=r;
	rgb_buffer[1]=g;
	rgb_buffer[2]=b;

	fb_buffer = (unsigned char*) convertRGB2FB(rgb_buffer, 1, var->bits_per_pixel, &cpp);
	m_busy_buffer = (unsigned char*) malloc(width*width*cpp);
	busy_buffer_wrk = m_busy_buffer;
	unsigned char* fb = CFrameBuffer::getInstance()->getFrameBufferPointer();
	for(int y=sy ; y < sy+width; y++)
   {
		for(int x=sx ; x< sx+width; x++)
	   {
			memcpy(busy_buffer_wrk, fb+y*var->xres*cpp + x*cpp, cpp);
			busy_buffer_wrk+=cpp;
			memcpy(fb+y*var->xres*cpp + x*cpp, fb_buffer, cpp);
		}
	}
	m_busy_x = sx;
	m_busy_y = sy;
	m_busy_width = width;
	m_busy_cpp = cpp;
	free(fb_buffer);
//	dbout("Show Busy}\n");
}
void CPictureViewer::hideBusy()
{
//	dbout("Hide Busy{\n");
	if(m_busy_buffer!=NULL)
	{
		struct fb_var_screeninfo *var;
		var = CFrameBuffer::getInstance()->getScreenInfo();
		unsigned char* fb = CFrameBuffer::getInstance()->getFrameBufferPointer();
		unsigned char* busy_buffer_wrk = m_busy_buffer;

		for(int y=m_busy_y ; y < m_busy_y+m_busy_width; y++)
		{
			for(int x=m_busy_x ; x< m_busy_x+m_busy_width; x++)
			{
				memcpy(fb+y*var->xres*m_busy_cpp + x*m_busy_cpp, busy_buffer_wrk, m_busy_cpp);
				busy_buffer_wrk+=m_busy_cpp;
			}
		}
		free(m_busy_buffer);
		m_busy_buffer=NULL;
	}
//	dbout("Hide Busy}\n");
}
