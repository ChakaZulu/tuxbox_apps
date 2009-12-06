/*
 * $Id: lcshot.c,v 1.1 2009/12/06 13:40:32 rhabarber1848 Exp $
 *
 * lcshot - d-box2 linux project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <byteswap.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mntent.h>
#include <errno.h>
#include <sys/utsname.h>

#include <sys/vt.h>
#include <png.h>
#include <linux/fb.h> 
#include "lcd.h"

#define PACKAGE 	"lcshot"
#define VERSION 	"1.3"
#define MAINTAINER_NAME "SnowHead"
#define MAINTAINER_ADDR "SnowHead@gmx.net"

#define X_RES 121
#define Y_RES 64

//#define DEBUG

/* some conversion macros */

struct picture{
  int xres,yres;
  char *buffer;
  struct fb_cmap *colormap;
  char bps,gray;
};

// Type fr manuelle Pixelfarbe
typedef struct
{
	__u8 red;
	__u8 green;
	__u8 blue;
} pxcolor;

pxcolor pixcolor={0xFF,0xFF,0xFF};
int zoom=1,compression=1,gray=0;

void FatalError(char* err){
  fprintf(stderr,"An error occured: %s %s\nExiting now...\n",err,strerror(errno));
  fflush(stderr);
  exit (1);
}

void Usage(char *binary){
  printf("Usage: %s [-c rrggbb] [ -z n] [ -s n] [ -u ] [ -g ] [-h] filename.png\n", binary);
}

void Help(char *binary){
    printf("lcshot - makes screenshots from LC-Display, v%s\n", VERSION);
    
    Usage(binary);

    printf("\nPossible options:\n");
    	printf("\t-h\t\tprint this usage information\n");
	printf("\t-c\t\tdefine pixelcolor in RGB-hex (e.g. -c d0d0d0)\n");
	printf("\t-z\t\tdefine zoomfactor for PNG\n");
	printf("\t-s\t\twait n seconds before grabbing\n");
	printf("\t-u\t\tsave PNG uncompressed\n");
	printf("\t-g\t\tsave PNG as grayscale\n");
}

static void ConvertColorstring (char *string, pxcolor *var)
{
	int red,green,blue;
	sscanf(string,"%2x%2x%2x",&red,&green,&blue);
	var->red=red;
	var->green=green;
	var->blue=blue;
}

int read_lc(struct picture *pict){
  int i,j,zx,zy;
 
  pict->xres=X_RES*zoom;
  pict->yres=Y_RES*zoom;
  pict->bps=1;
  pict->gray=gray;
  pict->buffer=malloc(X_RES*Y_RES*((gray)?1:3)*zoom*zoom);
  unsigned char *lbuf=pict->buffer;

  LCD_Init();

    for(i=0; i<Y_RES; i++)
    {
    	  for(zx=0; zx<zoom; zx++)
  	  {
	  	for(j=0; j<X_RES; j++)
  		{
			if(raw[j][i])
			{	
			  	for(zy=0; zy<zoom; zy++)
			  	{
					if(gray)
					{
			  			*(lbuf++)=0xFF;
					}
					else
					{
			  			*(lbuf++)=pixcolor.red;
						*(lbuf++)=pixcolor.green;
	  					*(lbuf++)=pixcolor.blue;
					}
  				}
  			}
  			else
  			{
  				lbuf+=((gray)?1:3)*zoom;
  			}
  		}
  	}
  }

  LCD_Close();

  pict->colormap=(struct fb_cmap*)malloc(sizeof(struct fb_cmap));
  pict->colormap->red=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
  pict->colormap->green=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
  pict->colormap->blue=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
  pict->colormap->transp=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
  pict->colormap->start=0;
  pict->colormap->len=1<<pict->bps;
  pict->colormap->red[1]=0xFF<<8; 
  pict->colormap->green[1]=0xFF<<8; 
  pict->colormap->blue[1]=0xFF<<8; 
  pict->colormap->transp[0]=0xFF<<8; 
  
  return 0;
}
  
static int Write_PNG(struct picture * pict, char *filename, int interlace, int gray){
  png_bytep *row_pointers;
  png_structp png_ptr;
  png_infop info_ptr;
  png_text txt_ptr[4];
  int i;
  int bit_depth=0, color_type;   
  FILE *OUTfd = fopen(filename, "wb");
#ifdef DEBUG
    i=open("pict.bin",O_CREAT|O_WRONLY|O_TRUNC);
    fprintf(stdout, "%i\n",write(i,(void*)pict->buffer,(pict->xres)*(pict->yres)));
    perror("dupa");
    fprintf(stdout, "Writing to %s %ix%i %i\n", filename,(pict->xres),(pict->yres),(pict->xres)*(pict->yres));
    fprintf(stdout, "start: %i, size: %i\n", pict->colormap->start,pict->colormap->len);
    fflush(stdout);
    close(i);
#endif

  if (!OUTfd)
    FatalError("couldn't open output file");

  png_ptr = png_create_write_struct(
        	PNG_LIBPNG_VER_STRING, 
        	(png_voidp)NULL, (png_error_ptr)NULL, (png_error_ptr)NULL);
        
  if (!png_ptr)
    FatalError("couldn't create PNG write struct.");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    FatalError("couldn't create PNG info struct.");
  }

/*  host_info=(struct utsname*)malloc(sizeof(struct utsname));
  uname(host_info);*/
  unsigned char pdate[21];
  time_t ptime=time(NULL);
  struct tm *ltime=localtime(&ptime);
  sprintf(pdate,"%04d/%02d/%02d %02d:%02d:%02d",ltime->tm_year+1900,ltime->tm_mon+1,ltime->tm_mday,ltime->tm_hour,ltime->tm_min,ltime->tm_sec);
  txt_ptr[0].key="Name";
  txt_ptr[0].text="LC-Display screenshot";
  txt_ptr[0].compression=PNG_TEXT_COMPRESSION_NONE;
  txt_ptr[1].key="Date";
  txt_ptr[1].text=pdate;
  txt_ptr[1].compression=PNG_TEXT_COMPRESSION_NONE;
  txt_ptr[2].key="Hostname";
  txt_ptr[2].text="DBox2";
  txt_ptr[2].compression=PNG_TEXT_COMPRESSION_NONE;
  txt_ptr[3].key="Program";
  txt_ptr[3].text=PACKAGE" v."VERSION" by "MAINTAINER_NAME;
  txt_ptr[3].compression=PNG_TEXT_COMPRESSION_NONE;

  png_set_text(png_ptr, info_ptr, txt_ptr, 4);

  png_init_io(png_ptr, OUTfd);
    
  png_set_compression_level(png_ptr, (compression)?Z_BEST_COMPRESSION:Z_NO_COMPRESSION);

  row_pointers=(png_bytep*)malloc(sizeof(png_bytep)*pict->yres);

  bit_depth=8;
  color_type=(gray)?PNG_COLOR_TYPE_GRAY:PNG_COLOR_TYPE_RGB;
  for (i=0; i<(pict->yres); i++)
    row_pointers[i]=pict->buffer+i*((gray)?1:3)*(pict->xres);
  png_set_invert_alpha(png_ptr);

  png_set_IHDR(png_ptr, info_ptr, pict->xres, pict->yres, 
      bit_depth, color_type, interlace, 
      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
  png_write_info(png_ptr, info_ptr);

  png_write_image(png_ptr, row_pointers);

  png_write_end(png_ptr, info_ptr);
  /* puh, done, now freeing memory... */
  png_destroy_write_struct(&png_ptr, &info_ptr);
   

  free(row_pointers);
  fclose(OUTfd);
  return 0;
 }

int main(int argc, char **argv){
  char *outfile = argv[argc-1];
  int opt,twait;
  struct picture pict;
  int interlace=PNG_INTERLACE_ADAM7;

  pict.colormap=NULL;
  
  while((opt = getopt(argc, argv, "hugc:z:s:")) != -1)
	{
    switch (opt){
    case 'h':
      Help(argv[0]);
      return 1;
      break;
 	case 'c':
 	 ConvertColorstring(optarg, &pixcolor);
  	 break;
 	case 'z':
 	 zoom=atoi(optarg);
  	 break;
 	case 'u':
 	 compression=0;
  	 break;
 	case 'g':
 	 gray=1;
  	 break;
	case 's':
         twait=(atoi(optarg));
         while(twait>0)
         {
      	   --twait;
      	   printf("[lcshot] Waiting for%4d s\r",twait);fflush(stdout);
      	   sleep(1);
         }
         printf("\n");
         break;
       default:
         break;
    }
  }

  if ((optind==argc) || (1!=argc-optind)){
    Usage(argv[0]);
    return 1;
  }

  read_lc(&pict);
      
  printf("[lcshot] Display content grabbed.\n");
  printf("[lcshot] Writing %s ...\n",outfile);fflush(stdout);
  Write_PNG(&pict, outfile, interlace, gray );

  if(pict.colormap){
    free(pict.colormap->red);
    free(pict.colormap->green);
    free(pict.colormap->blue);
    free(pict.colormap->transp);
    free(pict.colormap);
  }
  free(pict.buffer);
    
  printf ("[lcshot] done.\n");
    
  return 0;
}        
