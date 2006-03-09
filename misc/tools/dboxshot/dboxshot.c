/* $Id: dboxshot.c,v 1.1 2006/03/09 17:35:37 stdin Exp $ */

/* DBoxshot - This is a simple program that generates a screenshot of the
 * specified framebuffer device and terminal and writes it to a specified file
 * using the BMP format.
 * 
 * Copyright (C) 2006 Daniel Scheack (dscheack@t-online.de) 
 * 
 * Dboxshot is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. 
 * 
 * Dboxshot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License along with
 * Dboxshot; if not, write to the Free Software Foundation, Inc., 51 Franklin St,
 * Fifth Floor, Boston, MA 02110, USA 
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>

#define VER  "$Id: dboxshot.c,v 1.1 2006/03/09 17:35:37 stdin Exp $"
#define DEVICE "/dev/fb0"

#define true 1
#define false 0

unsigned short debug = false;

struct header {
	__u32 filesize;			// Dateigröße
	__u32 reserved;			// Null
	__u32 pic_pos;			// Pos. der Bilddaten
	__u32 header_l;			// Größe des Formatheaders
	__u32 xres;				// Bildbreite
	__u32 yres;				// Bildhöhe
	__u16 plane_sel;		// Anzahl der Bildebenen
	__u16 bpp;				// Bits pro Pixel
	__u32 comp;				// Kompression
	__u32 body_l;			// Größe der Pixeldaten
	__u32 pxm;				// hor. Auflösung in px/m
	__u32 pym;				// ver. Auflösung in px/m
	__u32 uc;				// benutzt Farben bei Palette
	__u32 ic;				// wichtige Farben bei Palette
};

struct header bmp_file_header;

struct picture {
	int xres,yres;
	int color_depth;
	char *buffer;
	struct fb_cmap *colormap;
};

// Type für manuelle Hintergrundfarbe
typedef struct
{
	__u8 red;
	__u8 green;
	__u8 blue;
} bgcolor;

static u_int32_t Long_Little2Big(u_int32_t var)
{
	u_int32_t tmp;
	unsigned char *ptmp;
	tmp = var;
	ptmp = (unsigned char *) &tmp;
	return(((u_int32_t) ptmp[3] << 24) + ((u_int32_t) ptmp[2] << 16) + ((u_int32_t) ptmp[1] << 8) + (u_int32_t) ptmp[0]);
}

static u_int16_t Short_Little2Big(u_int16_t var)
{
	u_int16_t tmp;
	unsigned char *ptmp;
	tmp = var;
	ptmp = (unsigned char *) &tmp;
	return(((u_int16_t) ptmp[1] << 8) + (u_int16_t) ptmp[0]);
}

static void Default_Header (struct header *bmp)
{
	bmp->reserved	= 0;
	bmp->pic_pos	= Long_Little2Big(1078);
	bmp->header_l	= Long_Little2Big(40);
	bmp->plane_sel	= Short_Little2Big(1);
	bmp->comp		= 0;
	bmp->uc			= 0;
	bmp->ic			= 0;
}

// Framebuffer einlesen
static void Read_FB (struct picture *pic)
{
	// Framebuffer Device öffen und Info auslesen
	int fd;
	if(!(fd=open(DEVICE, O_RDONLY)))
	{
		perror("Read_FB (fb device)");
		exit(EXIT_FAILURE);
	}
	
	struct fb_fix_screeninfo fb_fixinfo;
	struct fb_var_screeninfo fb_info;
    
    // Fixinfo für debug holen
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_fixinfo))
    {
    	perror("Read_FB (FBIOGET_FSCREENINFO)");
    	exit(EXIT_FAILURE);
    }
    
    // verwendete Auflösung holen
    if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_info))
    {
    	perror("Read_FB (FBIOGET_VSCREENINFO)");
    	exit(EXIT_FAILURE);
    }
    
    // Auflösung in struct setzen
    pic->xres = fb_info.xres;
    pic->yres = fb_info.yres;
    // Bits per Pixel setzen
    pic->color_depth = fb_info.bits_per_pixel;
    
    if (debug)
    {
    	printf("Read_FB (pic): xres: %d yres: %d color_depth: %d\n", pic->xres, pic->yres, pic->color_depth);
		printf("Read_FB (fb_info): %ix%i [%i,%i] %ibps %igr\n",
			fb_info.xres_virtual, fb_info.yres_virtual,
			fb_info.xoffset, fb_info.yoffset,
			fb_info.bits_per_pixel, fb_info.grayscale); 
		printf("Read_FB (fb_fixinfo): FIX: card: %s\n" 
			   "\tmem: 0x%.8X mem_len: %d visual: %i type: %i\n" 
			   "\ttype_aux: %i line_len: %i accel: %i\n",
			fb_fixinfo.id,fb_fixinfo.smem_start,fb_fixinfo.smem_len,fb_fixinfo.visual,
			fb_fixinfo.type,fb_fixinfo.type_aux,fb_fixinfo.line_length,fb_fixinfo.accel);
    }
    	    
    // Speicher für buffer allozieren
    if(!(pic->buffer=malloc(pic->xres*pic->yres)))
    {
    	perror("Read_FB (couldn't malloc buffer)");
    	exit(EXIT_FAILURE);
    }
    
    // Speicher für colormap allozieren
    pic->colormap = (struct fb_cmap *)malloc(sizeof(struct fb_cmap));
    pic->colormap->red = (__u16*) malloc(sizeof(__u16) * (1<<pic->color_depth));
    pic->colormap->green = (__u16*) malloc(sizeof(__u16) * (1<<pic->color_depth));
    pic->colormap->blue = (__u16*) malloc(sizeof(__u16) * (1<<pic->color_depth));
    pic->colormap->transp = (__u16*) malloc(sizeof(__u16) * (1<<pic->color_depth));
    pic->colormap->start = 0;
    pic->colormap->len = 1<<pic->color_depth;
    
    // Farbpalette holen
    if (ioctl(fd, FBIOGETCMAP, pic->colormap))
    {
    	perror("Read_FB (FBIOGETCMAP)");
    	exit(EXIT_FAILURE);
    }
	
	// Framebuffer auslesen
	if ((read(fd, pic->buffer, pic->xres*pic->yres)) != (pic->xres * pic->yres))
	{
		perror("Read_FB (couldn't read the framebuffer)");
    	exit(EXIT_FAILURE);
	}
	
	close(fd);
}

static u_int8_t Convert (u_int16_t var)
{
	return(var>>8);
}

static char *ReturnString (char *buffer, char *var)
{
	memcpy(var, buffer, 720);
	return(var);
}

static void SetBackground (struct picture *pic, bgcolor *color)
{
	if (debug)
		printf("SetBackground: Color-Palette-No: %i (R: %i G: %i B: %i)\n",(int)pic->buffer[0], color->red, color->green, color->blue);
		
	pic->colormap->blue[(int)pic->buffer[0]] = color->blue;
	pic->colormap->green[(int)pic->buffer[0]] = color->green;
	pic->colormap->red[(int)pic->buffer[0]] = color->red;
}

static void WriteBMP (char *filename, struct picture *pic)
{
	// Defaultwerte des BMP Hedaers setzen
	Default_Header(&bmp_file_header);
	
	bmp_file_header.filesize = Long_Little2Big(pic->xres*pic->yres+1078);
	bmp_file_header.xres = Long_Little2Big(pic->xres);
	bmp_file_header.yres = Long_Little2Big(pic->yres);
	bmp_file_header.bpp = Short_Little2Big(pic->color_depth);
	bmp_file_header.body_l = Long_Little2Big(pic->xres*pic->yres+1024);
	bmp_file_header.pxm = Long_Little2Big(pic->xres/0.254);
	bmp_file_header.pym = Long_Little2Big(pic->xres/0.254);
	
	
	FILE *fh;
	
	if (!(fh = fopen(filename, "wb")))
	{
		perror("WriteBMP (Filename)");
		exit(EXIT_FAILURE);
	}

	// Datei-Identifikation schreiben
	fprintf(fh, "BM");
	// Header speichern
	fwrite(&bmp_file_header, sizeof(struct header), 1, fh);
	// Farbpalette speichern
	int i = 0;
	char blue;
	char green;
	char red;
	char transp;
	for(; i<pic->colormap->len; i++)
	{
		blue = Convert(pic->colormap->blue[i]<<8);
		green = Convert(pic->colormap->green[i]<<8);
		red = Convert(pic->colormap->red[i]<<8);
		transp = Convert(pic->colormap->transp[i]<<8);

    	fwrite(&blue, 1, 1, fh);
    	fwrite(&green, 1, 1, fh);
    	fwrite(&red, 1, 1, fh);
    	fwrite(&transp, 1, 1, fh);
	}
	// Datenpixel speichern
	long x;
	char tmp[720];
	
	for (i=pic->yres; i>0; i--)
	{
		x = (i*pic->xres) - pic->xres;	// Anfang
		ReturnString(&pic->buffer[x], tmp);
		fwrite(&tmp, 720, 1, fh);
	}
		
	fclose(fh);
}

static void ConvertColorstring (char *string, bgcolor *var)
{
	char r[] = {string[0], string[1], '\0'}; // R
	char g[] = {string[2], string[3], '\0'}; // G
	char b[] = {string[4], string[5], '\0'}; // B
	
	var->red = strtol(r, NULL, 16);
	var->green = strtol(g, NULL, 16);
	var->blue = strtol(b, NULL, 16);
}

static void Memory_free (struct picture *pic)
{
	// Speicher für Buffer freigeben
	free(pic->buffer);
	// Speicher für Farbelemente freigeben
	if(pic->colormap)
	{
		free(pic->colormap->red);
		free(pic->colormap->green);
		free(pic->colormap->blue);
		free(pic->colormap->transp);
		free(pic->colormap);
	}
}

// Programminfo ausgeben
static void Usage ()
{
	printf(
	"DBoxShot (c) by Daniel Scheack <dscheack@t-online.de>\n"
	"%s\n"
	"Usage: dboxshot [-d] [-h] [-c hex] [-t n] [-o filename]\n"
	"  Options\n"
	"  -d\tDebugmodus enabled\n"
	"  -h\tprint out this message\n"
	"  -c\tdefine backgroundcolor in RGB-hex (e.g. -c d0d0d0)\n"
	"  -t\twait mSec. before Screenshot\n"
	"  -o\tdefine Outputfilename\n"
	"If filename not indicated, the output file is called fbout.bmp\n",
	VER);
}

int main (int argc, char* argv[])
{
	int opt;
	char *filename = "fbout.bmp";
	bgcolor bg;
	int bgflag = false;
	int timeshot = false;
	
	// Argumente einlesen
	while((opt = getopt(argc, argv, "d?ho:c:t:")) != -1)
	{
		switch (opt)
		{
			case 't':
				timeshot = atoi(optarg) * 100000;
				break;
			case 'c':
				ConvertColorstring(optarg, &bg);
				bgflag = true;
				break;
			case 'o':
				filename = optarg;
				break;
			case 'd':
				debug = true;
				break;
			case 'h':
				Usage();
				exit(EXIT_SUCCESS);
				break;
			case '?':
				Usage();
				exit(EXIT_SUCCESS);
				break;
			default:
				exit(EXIT_FAILURE);
		}
	}
	
	if (debug)
		printf("Debugmode enabled\nfilename: %s\n", filename);
	
	// Screenshot nach Angabe von Millisekunden
	if (timeshot>0)
	{
		if (debug)
			printf("Wait %i msec. ...\n", timeshot); 
		usleep(timeshot);
	}
		
	struct picture pic;
	
	// Framebuffer auslesen
	Read_FB(&pic);
	
	// übergebende Hintergrundfarbe setzen
	if (bgflag == true)
		SetBackground(&pic, &bg);
	
	// BMP-Bild speichern
	WriteBMP(filename, &pic);
	
	// Speicher freigeben
	Memory_free(&pic);
	
	exit(EXIT_SUCCESS);
}
