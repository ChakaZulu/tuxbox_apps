/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
        baseroutines by Shadow_
	Homepage: http://dbox.cyberphoria.org/



	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "lcddisplay.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>


CLCDDisplay::CLCDDisplay()
{
	paused=0;
	available = false;
	//open device
	if((fd = open("/dev/dbox/lcd0",O_RDWR)) < 0)
	{
		perror("LCD (/dev/dbox/lcd0)");
		return;
	}

	//clear the display
	if ( ioctl(fd,LCD_IOCTL_CLEAR) < 0)
	{
		perror("clear failed");
		return;
	}
	
	//graphic (binary) mode 
	int i=LCD_MODE_BIN;
	if ( ioctl(fd,LCD_IOCTL_ASC_MODE,&i) < 0 )
	{
		perror("graphic mode failed");
		return;
	}

	iconBasePath = "";
	available = true;
}

bool CLCDDisplay::isAvailable()
{
	return available;
}

CLCDDisplay::~CLCDDisplay()
{
	close(fd);
}

void CLCDDisplay::pause()
{
	paused = 1;
}

void CLCDDisplay::resume()
{
	//clear the display
	if ( ioctl(fd,LCD_IOCTL_CLEAR) < 0)
	{
		perror("clear failed");
		return;
	}
	
	//graphic (binary) mode 
	int i=LCD_MODE_BIN;
	if ( ioctl(fd,LCD_IOCTL_ASC_MODE,&i) < 0 )
	{
		perror("graphic mode failed");
		return;
	}

	paused = 0;
}

void CLCDDisplay::convert_data ()
{
	unsigned int x, y, z;
	char tmp;

	for (x = 0; x < LCD_COLS; x++)
	{   
		for (y = 0; y < LCD_ROWS; y++)
		{
			tmp = 0;

			for (z = 0; z < 8; z++)
				if (raw[y * 8 + z][x] == 1)
					tmp |= (1 << z);

			lcd[y][x] = tmp;
		}
	}
}

void CLCDDisplay::update()
{
	convert_data();
	if(paused)
		return;
	else
		if ( write(fd, lcd, LCD_BUFFER_SIZE) < 0) {
			perror("lcdd: CLCDDisplay::update(): write()");
		}
}

int CLCDDisplay::sgn (int arg) 
{
	if(arg<0)
		return -1;
	if(arg>0)
		return 1;
	return 0;
}


void CLCDDisplay::draw_point (int x,int y, int state)
{
	if ((x<0) || (x>=LCD_COLS) || (y<0) || (y>=(LCD_ROWS*8))) return;
	if(state == LCD_PIXEL_INV)
	{
		/* aus 1 mach 0 und aus 0 mach 1 */
		raw[y][x] = LCD_PIXEL_ON - raw[y][x];
	}
	else
		raw[y][x] = state;
}


/*
 * draw_line
 * 
 * args:
 * x1    StartCol
 * y1    StartRow
 * x2    EndCol
 * y1    EndRow
 * state LCD_PIXEL_OFF/LCD_PIXEL_ON/LCD_PIXEL_INV
 * 
 */

void CLCDDisplay::draw_line (int x1, int y1, int x2, int y2, int state)  
{   
	int dx,dy,sdx,sdy,px,py,dxabs,dyabs,i;
	float slope;
   
	dx=x2-x1+1;      
	dy=y2-y1+1;      
	dxabs=abs(dx);
	dyabs=abs(dy);
	sdx=sgn(dx);
	sdy=sgn(dy);
	if (dxabs>=dyabs) /* the line is more horizontal than vertical */ {
		slope=(float)dy / (float)dx;
		for(i=0;i!=dx;i+=sdx) {	     
			px=i+x1;
			py=int( slope*i+y1 );
			draw_point(px,py,state);
		}
	}
	else /* the line is more vertical than horizontal */ {	
		slope=(float)dx / (float)dy;
		for(i=0;i!=dy;i+=sdy) {
			px=int(slope*i+x1);
			py=i+y1;
			draw_point(px,py,state);
		}
	}
}


void CLCDDisplay::draw_fill_rect (int left,int top,int right,int bottom,int state) {
	int x,y;
	for(x = left + 1;x < right;x++) {  
		for(y = top + 1;y < bottom;y++) {
			draw_point(x,y,state);
		}
	}
}


void CLCDDisplay::draw_rectangle (int left,int top, int right, int bottom, int linestate,int fillstate)
{
	// coordinate checking in draw_pixel (-> you can draw lines only
	// partly on screen)

	draw_line(left,top,right,top,linestate);
	draw_line(left,top,left,bottom,linestate);
	draw_line(right,top,right,bottom,linestate);
	draw_line(left,bottom,right,bottom,linestate);
	draw_fill_rect(left,top,right,bottom,fillstate);  
}  


void CLCDDisplay::draw_polygon(int num_vertices, int *vertices, int state) 
{

	// coordinate checking in draw_pixel (-> you can draw polygons only
	// partly on screen)

	int i;
	for(i=0;i<num_vertices-1;i++) {
		draw_line(vertices[(i<<1)+0],
			vertices[(i<<1)+1],
			vertices[(i<<1)+2],
			vertices[(i<<1)+3],
			state);
	}
   
	draw_line(vertices[0],
		vertices[1],
		vertices[(num_vertices<<1)-2],
		vertices[(num_vertices<<1)-1],
		state);
}



bool CLCDDisplay::paintIcon(std::string filename, int x, int y, bool invert)
{
	short width, height;
	unsigned char tr;

	int fd;
	filename = iconBasePath + filename;

	fd = open(filename.c_str(), O_RDONLY );
	
	if (fd==-1)
	{
		printf("\nerror while loading icon: %s\n\n", filename.c_str() );
		return false;
	}

	read(fd, &width,  2 );
	read(fd, &height, 2 );
	read(fd, &tr, 1 );

	width= ((width & 0xff00) >> 8) | ((width & 0x00ff) << 8);
	height=((height & 0xff00) >> 8) | ((height & 0x00ff) << 8);

	unsigned char pixbuf[200];
	for (int count=0; count<height; count ++ )
	{
		read(fd, &pixbuf, width >> 1 );
		unsigned char *pixpos = (unsigned char*) &pixbuf;
		for (int count2=0; count2<width >> 1; count2 ++ )
		{
			unsigned char compressed = *pixpos;
			unsigned char pix1 = (compressed & 0xf0) >> 4;
			unsigned char pix2 = (compressed & 0x0f);

			if ((pix1 != tr) ^ invert)
				draw_point(x+(count2<<1),y+count, PIXEL_ON);
			else
				draw_point (x+(count2<<1),y+count, PIXEL_OFF);
			if ((pix2 != tr) ^ invert)
				draw_point(x+(count2<<1)+1,y+count, PIXEL_ON);
			else
				draw_point (x+(count2<<1)+1,y+count, PIXEL_OFF);
			pixpos++;
		}
	}
	
	close(fd);
	return true;
}

void CLCDDisplay::dump_screen(raw_display_t *screen) {
	memcpy(screen, raw, sizeof(raw_display_t));
}

void CLCDDisplay::load_screen(raw_display_t *screen) {
	memcpy(raw, screen, sizeof(raw_display_t));
}
