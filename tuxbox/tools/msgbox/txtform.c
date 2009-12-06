/*
 * $Id: txtform.c,v 1.1 2009/12/06 21:58:11 rhabarber1848 Exp $
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

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "text.h"
#include "gfx.h"
#include "msgbox.h"
#include "color.h"

#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

int fh_txt_trans(const char *name, int xs, int xw, int ys, int dy, int cs, int line, int *cut, int *x, int *y, int plot)
{
char tstr[BUFSIZE],rstr[BUFSIZE],*tptr;
int loop=1, j, first, slen, cnt=0;
FILE *fh;
int just, color=CMCT;

	if(!(fh=fopen(name,"rb")))	return(FH_ERROR_FILE);

	first=(line==0);
	*x=0;
	*y=0;
	while((loop>0) && (fgets(tstr, sizeof(tstr), fh)))
	{
		j=0;
		just=LEFT;
		color=CMCT;
		
		tptr=tstr+strlen(tstr);
		while((tptr>=tstr) && (*tptr<=32))
		{
			*tptr=0;
			--tptr;
		}
		tptr=tstr;
		while(*tptr)
		{
			rstr[j++]=*tptr;
			cnt++;

			if(*tptr == '~')
			{
				switch (*(tptr+1))
				{
					case 'l': just=LEFT; break;
					case 'r': just=RIGHT; break;
					case 'c': just=CENTER; break;
					case 's':
						RenderBox(xs, ys-cs/3, xs+xw, ys-cs/3+1, FILL, COL_MENUCONTENT_PLUS_3);
						RenderBox(xs, ys-cs/3+1, xs+xw, ys-cs/3+2, FILL, COL_MENUCONTENT_PLUS_1);
						break;
				}
			}
			tptr++;
		}
		if((loop>0) && (ys<(ey-dy)))
		{
			rstr[j]=0;
			if(plot)
			{
				if(loop>=line)
				{
					RenderString(rstr, xs, ys, xw, just, cs, color);
					if(strlen(rstr))
					{
						first=0;
					}
					ys+=dy;
				}
			}
			else
			{
				if(strlen(rstr))
				{
					slen=GetStringLen(xs,rstr);
					if(slen>*x)
					{
						*x=slen;
					}
				}
				*y=*y+1;
			}
		}
	}
	if(plot)
	{
		*cut=(ys>=(ey-dy));
	}
	fclose(fh);
	return(FH_ERROR_OK);
}

int fh_txt_load(const char *name, int sx, int wx, int sy, int dy, int cs, int line, int *cut)
{
int dummy;

	return fh_txt_trans(name, sx, wx, sy, dy, cs, line, cut, &dummy, &dummy, 1);
}


int fh_txt_getsize(const char *name, int *x, int *y, int *cut)
{
	return fh_txt_trans(name, 0, 0, 0, 0, 0, 0, cut, x, y, 0);
}
