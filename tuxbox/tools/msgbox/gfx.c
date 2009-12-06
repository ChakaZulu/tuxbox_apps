/*
 * $Id: gfx.c,v 1.1 2009/12/06 21:58:11 rhabarber1848 Exp $
 *
 * msgbox - d-box2 linux project
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

#include "msgbox.h"

/******************************************************************************
 * RenderBox
 ******************************************************************************/

void RenderBox(int sx, int sy, int ex, int ey, int rad, int col)
{
	int F,R=rad,ssx=startx+sx,ssy=starty+sy,dxx=ex-sx,dyy=ey-sy,rx,ry,wx,wy,count;

	unsigned char *pos=(lbb+ssx+var_screeninfo.xres*ssy);
	unsigned char *pos0, *pos1, *pos2, *pos3;
	
	if (dxx<0) 
	{
		printf("[shellexec] RenderBox called with dx < 0 (%d)\n", dxx);
		dxx=0;
	}

	if(R)
	{
		if(--dyy<=0)
		{
			dyy=1;
		}

		if(R==1 || R>(dxx/2) || R>(dyy/2))
		{
			R=dxx/10;
			F=dyy/10;	
			if(R>F)
			{
				if(R>(dyy/3))
				{
					R=dyy/3;
				}
			}
			else
			{
				R=F;
				if(R>(dxx/3))
				{
					R=dxx/3;
				}
			}
		}
		ssx=0;
		ssy=R;
		F=1-R;

		rx=R-ssx;
		ry=R-ssy;

		pos0=pos+((dyy-ry)*var_screeninfo.xres);
		pos1=pos+(ry*var_screeninfo.xres);
		pos2=pos+(rx*var_screeninfo.xres);
		pos3=pos+((dyy-rx)*var_screeninfo.xres);
		while (ssx <= ssy)
		{
			rx=R-ssx;
			ry=R-ssy;
			wx=rx<<1;
			wy=ry<<1;

			memset(pos0+rx, col, dxx-wx);
			memset(pos1+rx, col, dxx-wx);
			memset(pos2+ry, col, dxx-wy);
			memset(pos3+ry, col, dxx-wy);

			ssx++;
			pos2-=var_screeninfo.xres;
			pos3+=var_screeninfo.xres;
			if (F<0)
			{
				F+=(ssx<<1)-1;
			}
			else   
			{ 
				F+=((ssx-ssy)<<1);
				ssy--;
				pos0-=var_screeninfo.xres;
				pos1+=var_screeninfo.xres;
			}
		}
		pos+=R*var_screeninfo.xres;
	}

	for (count=R; count<(dyy-R); count++)
	{
		memset(pos, col, dxx);
		pos+=var_screeninfo.xres;
	}
}


