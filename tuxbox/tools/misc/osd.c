/*
 *   osd.c - AViA OSD application (dbox-II-project)
 *
 *   Homepage: http://dbox2.elxsi.de
 *
 *   Copyright (C) 2001 Gillem (htoa@gmx.net)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *   $Log: osd.c,v $
 *   Revision 1.1  2001/03/06 21:49:23  gillem
 *   - initial release
 *
 *
 *
 *   $Revision: 1.1 $
 *
 */

/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/types.h>

#include "dbox/avia_osd.h"
#include "tuxbox.h"

/* ---------------------------------------------------------------------- */

void rgb2crycb( int r, int g, int b, int blend, __u32 * pale )
{
	int cr,y,cb;

	if(!pale)
		return;

	y  = ((257*r  + 504*g + 98*b)/1000 + 16)&0x7f;
	cr = ((439*r  - 368*g - 71*b)/1000 + 128)&0x7f;
	cb = ((-148*r - 291*g + 439*b)/1000 + 128)&0x7f;

	*pale = (y<<16)|(cr<<9)|(cb<<2)|(blend&3);

	printf("OSD DATA: %d %d %d\n",cr,y,cb);
}

/* ---------------------------------------------------------------------- */

int main (int argc, char **argv)
{
	int fd;
	int x,y,z,a;

	struct sosd_create_frame osdf;
	__u32 palette[16];
	__u32 bitmap[20000];
	__u32 i;
	__u32 pale;

	if ((fd = open("/dev/dbox/osd0",O_RDWR)) <= 0)
	{
		perror("open");
		return -1;
	}

	memset(bitmap,0,60000);

	osdf.framenr = 0;
	osdf.x = 100;
	osdf.y = 100;
	osdf.w = 120-1;
	osdf.h = 64*2; // odd + even
    osdf.gbf = 0x1f;
	osdf.pel = 1; // 4 bit * pixel

	rgb2crycb( 100, 0, 100, 0, &pale );
	palette[0] = 0;//pale;
	rgb2crycb( 110, 0, 110, 3, &pale );

	/* set palette */
	for(i=1;i<16;i++)
	{
		palette[i] = pale;
	}

	y=0;z=0;i=0;
	for(x=0;x<120*64;x++)
	{
		i |= header_data[x]<<(28-(y*4));
//		printf("%08X %d %d\n",i,y,header_data[x]);
		y++;
		if(y==8)
		{
			bitmap[z] = i;
			z++;
			y=0;
			i=0;
		}
	}

	printf("%d %d\n",z,x);

	osdf.psize = 16;
	osdf.palette = &palette;
	osdf.bsize = 120*64*4/16;	// half words ??? f*ck
	osdf.bitmap = &bitmap;

	if ( ioctl( fd, OSD_IOCTL_CREATE_FRAME, &osdf ) < 0 )
	{
		perror("ioctl");
	}

	close(fd);

  return 0;
}
