/*
 * $Id: gfx.c,v 1.1 2009/12/20 16:22:58 rhabarber1848 Exp $
 *
 * sysinfo - d-box2 linux project
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

#include "sysinfo.h"

#ifndef abs
#define abs(a)	((a>0)?a:-(a))
#endif

int stride = 720;

typedef struct { unsigned char width_lo; unsigned char width_hi; unsigned char height_lo; unsigned char height_hi; 	unsigned char transp; } IconHeader;

/******************************************************************************
 * RenderBox
 ******************************************************************************/

void RenderBox(int sx, int sy, int ex, int ey, int mode, int color)
{
	int loop;
	stride = fix_screeninfo.line_length;
	if(mode == FILL) for(; sy <= ey; sy++) memset(lbb + sx + var_screeninfo.xres*(sy), color, ex-sx + 1);
	else
	{
		//hor lines

			for(loop = sx; loop <= ex; loop++)
			{
				*(lbb + loop + var_screeninfo.xres*(sy)) = color;
				*(lbb + loop + var_screeninfo.xres*(sy+1)) = color;

				*(lbb + loop + var_screeninfo.xres*(ey-1)) = color;
				*(lbb + loop + var_screeninfo.xres*(ey)) = color;
			}

		//ver lines

			for(loop = sy; loop <= ey; loop++)
			{
				*(lbb + sx + var_screeninfo.xres*(loop)) = color;
				*(lbb + sx+1 + var_screeninfo.xres*(loop)) = color;

				*(lbb + ex-1 + var_screeninfo.xres*(loop)) = color;
				*(lbb + ex + var_screeninfo.xres*(loop)) = color;
			}
	}
}

void	FBPaintPixel( int x, int y,int width, int farbe )
{
	y=(int)(y-(width/2));
	int i=0;
	for(i=0;i<width;i++) *(lbb + stride*(y+i) + x) = farbe;
}
void	RenderLine( int xa, int ya, int xb, int yb, int width, int farbe )
{
	int 	dx = abs (xa - xb);
	int	dy = abs (ya - yb);
	int	x;
	int	y;
	int	End;
	int	step;

	if ( dx > dy )
	{
		int	p = 2 * dy - dx;
		int	twoDy = 2 * dy;
		int	twoDyDx = 2 * (dy-dx);

		if ( xa > xb )
		{
			x = xb;
			y = yb;
			End = xa;
			step = ya < yb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = xb;
			step = yb < ya ? -1 : 1;
		}

		FBPaintPixel (x, y,width, farbe);

		while( x < End )
		{
			x++;
			if ( p < 0 )
				p += twoDy;
			else
			{
				y += step;
				p += twoDyDx;
			}
			FBPaintPixel (x, y,width, farbe);
		}
	}
	else
	{
		int	p = 2 * dx - dy;
		int	twoDx = 2 * dx;
		int	twoDxDy = 2 * (dx-dy);

		if ( ya > yb )
		{
			x = xb;
			y = yb;
			End = ya;
			step = xa < xb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = yb;
			step = xb < xa ? -1 : 1;
		}

		FBPaintPixel (x, y,width, farbe);

		while( y < End )
		{
			y++;
			if ( p < 0 )
				p += twoDx;
			else
			{
				x += step;
				p += twoDxDy;
			}
			FBPaintPixel (x, y,width, farbe);
		}
	}
}

void	RenderHLine( int sx, int sy, int ex, char dot, char width, char spacing, int farbe )
{
	if (dot == 0)  RenderBox(sx,(int)(sy-(width/2)),ex, (int)(sy+(width/2)), FILL, farbe);
	if (dot == 1)
	{
		while (sx <= ex)
		{
			RenderBox(sx,(int)(sy-(width/2)),sx+width, (int)(sy+(width/2)), FILL, farbe);
			sx=sx+width+(spacing*width);
		}
	}
}

void	RenderVLine( int sx, int sy, int ey, char dot, char width, char spacing, int farbe )
{
	if (dot == 0)  RenderBox((int)(sx-(width/2)),sy,(int)(sx+(width/2)),ey , FILL, farbe);
	if (dot == 1)
	{
		while (sy <= ey)
		{
			RenderBox((int)(sx-(width/2)),sy,(int)(sx+(width/2)),sy+width , FILL, farbe);
			sy=sy+width+(spacing*width);
		}
	}
}

/******************************************************************************
 * PaintIcon
 ******************************************************************************/
void PaintIcon(char *filename, int x, int y, unsigned char offset)
{
	IconHeader iheader;
	unsigned int  width, height,count,count2;
	unsigned char pixbuf[768],*pixpos,compressed,pix1,pix2;
	unsigned char * d = (lbb+x+var_screeninfo.xres*y);
	unsigned char * d2;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd == -1)
	{
		printf("shellexec <unable to load icon: %s>\n", filename);
		return;
	}

	read(fd, &iheader, sizeof(IconHeader));

	width  = (iheader.width_hi  << 8) | iheader.width_lo;
	height = (iheader.height_hi << 8) | iheader.height_lo;


	for (count=0; count<height; count ++ )
	{
		read(fd, &pixbuf, width >> 1 );
		pixpos = (unsigned char*) &pixbuf;
		d2 = d;
		for (count2=0; count2<width >> 1; count2 ++ )
		{
			compressed = *pixpos;
			pix1 = (compressed & 0xf0) >> 4;
			pix2 = (compressed & 0x0f);

			if (pix1 != iheader.transp)
			{
				*d2=pix1 + offset;
			}
			d2++;
			if (pix2 != iheader.transp)
			{
				*d2=pix2 + offset;
			}
			d2++;
			pixpos++;
		}
		d += var_screeninfo.xres;
	}
	close(fd);
	return;
}
