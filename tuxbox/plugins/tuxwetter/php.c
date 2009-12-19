/*
 * $Id: php.c,v 1.1 2009/12/19 19:42:49 rhabarber1848 Exp $
 *
 * tuxwetter - d-box2 linux project
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
#include "tuxwetter.h"

#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

int fh_php_trans(const char *name, int sx, int sy, int dy, int cs, int line, int highlite, int *cut, int *x, int *y, int plain, int plot)
{
char tstr[BUFSIZE],rstr[BUFSIZE],*tptr,*xptr,cc,br3flag=0;
int loop=1, j, first, aline=0, fy=sy, slen, deg=0;
FILE *fh;

	if(!(fh=fopen(name,"rb")))	return(FH_ERROR_FILE);

	first=(line==0);
	*x=0;
	*y=0;
	while((loop>0) && (fgets(tstr, sizeof(tstr), fh)))
	{
		tptr=tstr+strlen(tstr);
		while((tptr>=tstr) && (*tptr<=32))
		{
			*tptr=0;
			--tptr;
		}

		if(((tptr=strstr(tstr,"<br>"))!=NULL) || ((tptr=strstr(tstr,"<h3>"))!=NULL))
		{
			tptr+=4;
			if((xptr=strstr(tstr,"</h3>"))!=NULL)
			{
				*xptr=0;
				br3flag=1;
			}
			if((*tptr=='=') || (strncmp(tptr,"<br>",4)==0))
			{
				if(aline>=line)
				{
					first=1;
				}
			}
			else
			{
				if(aline++>=line)
				{
					j=0;
					while(*tptr)
					{
						if(plain || (*tptr != '&'))
						{
							rstr[j++]=*tptr;
							tptr++;
						}
						else
						{
							if((*(tptr+1)!='#') && (strstr(tptr,"uml;")!=(tptr+2)) && (strstr(tptr,"nbsp;")!=(tptr+1)) && (strstr(tptr,"gt;")!=(tptr+1)) && (strstr(tptr,"lt;")!=(tptr+1)) && (strstr(tptr,"quot;")!=(tptr+1)) && (strstr(tptr,"zlig;")!=(tptr+2)))
							{
								rstr[j++]=*tptr++;
							}
							else
							{
								tptr++;
								cc=' ';
								switch (*tptr)
								{
									case 'a': cc='ä'; break;
									case 'A': cc='Ä'; break;
									case 'o': cc='ö'; break;
									case 'O': cc='Ö'; break;
									case 'u': cc='ü'; break;
									case 'U': cc='Ü'; break;
									case 's': cc='ß'; break;
									case 'q':
									case 'Q': cc='"'; break;
									case 'l':
									case 'g': cc=0;   break;
									case '#': 
										if(sscanf(tptr+1,"%3d",&deg)==1)
										{
											cc=deg;
										}
										break;
								}
								if(cc)
								{
									rstr[j++]=cc;
								}
								if((tptr=strchr(tptr,';'))==NULL)
								{
									printf("Tuxwetter <Parser Error in PHP>\n");
									fclose(fh);
									return -1;
								}
								else
								{
									++tptr;
								}
							}
						}
					}
					if((loop>0) && (sy<500))
					{
						rstr[j]=0;
						if(plot)
						{
							if(!br3flag)
							{
								RenderString(rstr, sx, sy, 619-sx, LEFT, cs, (first && highlite)?GREEN:CMCT);
							}
							else
							{
								RenderString(rstr, sx, 250, 619-sx, CENTER, BIG, CMCT);
							}
							if(strlen(rstr))
							{
								first=0;
							}
							sy+=dy;
						}
						else
						{
							if(strlen(rstr))
							{
								slen=GetStringLen(rstr);
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
					*cut=(sy>=500);
					if(line)
					{
						RenderString("<<", 0, fy, sx, CENTER, 56, CMHT);
					}
					if(*cut)
					{
						RenderString(">>", 0, sy-dy, sx, CENTER, 56, CMHT);
					}
				}
			}
		}
	}
	fclose(fh);
	return(FH_ERROR_OK);
}

int fh_php_load(const char *name, int sx, int sy, int dy, int cs, int line, int highlite, int plain, int *cut)
{
	int dummy;
	
	return fh_php_trans(name, sx, sy, dy, cs, line, highlite, cut, &dummy, &dummy, plain, 1);
}


int fh_php_getsize(const char *name, int plain, int *x, int *y)
{
	int dummy;
	
	return fh_php_trans(name, 0, 0, 0, 0, 0, 0, &dummy, x, y, plain, 0);
}
