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


#ifdef FBV_SUPPORT_GIF
    extern int fh_gif_getsize(const char *,int *,int*);
    extern int fh_gif_load(const char *,unsigned char *,int,int);
    extern int fh_gif_id(const char *);
#endif
#ifdef FBV_SUPPORT_JPEG
    extern int fh_jpeg_getsize(const char *,int *,int*);
    extern int fh_jpeg_load(const char *,unsigned char *,int,int);
    extern int fh_jpeg_id(const char *);
#endif
#ifdef FBV_SUPPORT_PNG
    extern int fh_png_getsize(const char *,int *,int*);
    extern int fh_png_load(const char *,unsigned char *,int,int);
    extern int fh_png_id(const char *);
#endif
#ifdef FBV_SUPPORT_BMP
    extern int fh_bmp_getsize(const char *,int *,int*);
    extern int fh_bmp_load(const char *,unsigned char *,int,int);
    extern int fh_bmp_id(const char *);
#endif

void CPictureViewer::add_format(int (*picsize)(const char *,int *,int*),int (*picread)(const char *,unsigned char *,int,int), int (*id)(const char*))
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

CPictureViewer::CFormathandler * CPictureViewer::fh_getsize(const char *name,int *x,int *y)
{
	CFormathandler *fh;
    for(fh=fh_root;fh!=NULL;fh=fh->next)
    {
	if(fh->id_pic(name))
	    if(fh->get_size(name,x,y)==FH_ERROR_OK) return(fh);
    }
    return(NULL);
}

bool CPictureViewer::DecodeImage(std::string name)
{
   dbout("DecodeImage {\n"); 
	int x,y,xs,ys,imx,imy;
   CFormathandler *fh;
   if((fh=fh_getsize(name.c_str(),&x,&y)))
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
		
      dbout("---Decoding Start(%d/%d)\n",x,y);
		if(fh->get_pic(name.c_str(),m_Pic_Buffer,x,y)==FH_ERROR_OK)
		{
			dbout("---Decoding Done\n");
			getCurrentRes(&xs,&ys);
			if((x>xs || y>ys) && m_scaling!=NONE)
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
				dbout("---Scaling Start\n");
				if(m_scaling==SIMPLE)
					m_Pic_Buffer=simple_resize(m_Pic_Buffer,x,y,imx,imy);
				else
					m_Pic_Buffer=color_average_resize(m_Pic_Buffer,x,y,imx,imy);
				dbout("---Scaling Done\n");
				x=imx; y=imy;
			}
         m_Pic_Name = name;
         m_Pic_X=x;
         m_Pic_Y=y;
			if(x<xs) 
            m_Pic_XPos=(xs-x)/2; 
         else 
            m_Pic_XPos=0;
			if(y<ys) 
            m_Pic_YPos=(ys-y)/2; 
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
			m_Pic_Buffer=(unsigned char *) malloc(x*y*3);
			memset(m_Pic_Buffer, 0 , x*y*3);
         m_Pic_X=0;
         m_Pic_Y=0;
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
		m_Pic_Buffer=(unsigned char *) malloc(x*y*3);
		memset(m_Pic_Buffer, 0 , x*y*3);
		m_Pic_X=0;
		m_Pic_Y=0;
		m_Pic_XPos=0;
		m_Pic_YPos=0;
		m_Pic_XPan=0;
		m_Pic_YPan=0;
		printf("Unable to read file or format not recognized!\n");
   }
	
   dbout("DecodeImage }\n"); 
	return(m_Pic_Buffer!=NULL);
}


bool CPictureViewer::ShowImage(std::string filename)
{
   if(filename!=m_Pic_Name)
   {
      // Picture not yet decoded
      DecodeImage(filename);
   }
   DisplayImage();
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

