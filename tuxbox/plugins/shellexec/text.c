/*
 * $Id: text.c,v 1.2 2009/12/27 12:08:02 rhabarber1848 Exp $
 *
 * shellexec - d-box2 linux project
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

#include "color.h"
#include "text.h"
#include "gfx.h"
#include "io.h"

int FSIZE_BIG=40;
int FSIZE_MED=30;
int FSIZE_SMALL=24;
int TABULATOR=40;
static unsigned sc[8]={'a','o','u','A','O','U','z','d'}, tc[8]={'ä','ö','ü','Ä','Ö','Ü','ß','°'}, su[7]={0xA4,0xB6,0xBC,0x84,0x96,0x9C,0x9F};

void TranslateString(char *src)
{
int i,found,quota=0;
char rc,*rptr=src,*tptr=src;

	while(*rptr != '\0')
	{
		if(*rptr=='\'')
		{
			quota^=1;
		}
		if(!quota && *rptr=='~')
		{
			++rptr;
			rc=*rptr;
			found=0;
			for(i=0; i<sizeof(sc) && !found; i++)
			{
				if(rc==sc[i])
				{
					rc=tc[i];
					found=1;
				}
			}
			if(found)
			{
				*tptr=rc;
			}
			else
			{
				*tptr='~';
				tptr++;
				*tptr=*rptr;
			}
		}
		else
		{
			if (!quota && *rptr==0xC3 && *(rptr+1))
			{
				found=0;
				for(i=0; i<sizeof(su) && !found; i++)
				{
					if(*(rptr+1)==su[i])
					{
						found=1;
						*tptr=tc[i];
						++rptr;
					}
				}
				if(!found)
				{
					*tptr=*rptr;
				}
			}
			else
			{
				*tptr=*rptr;
			}
		}
		tptr++;
		rptr++;
	}
	*tptr=0;
}
/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

	if(!result) printf("<Font \"%s\" loaded>\n", (char*)face_id);
	else        printf("<Font \"%s\" failed>\n", (char*)face_id);

	return result;
}

int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, int color)
{
	int row, pitch, bit, x = 0, y = 0;
	FT_UInt glyphindex;
	FT_Vector kerning;

	//load char

		if(!(glyphindex = FT_Get_Char_Index(face, currentchar)))
		{
			printf("<FT_Get_Char_Index for Char \"%c\" failed with Errorcode 0x%.2X>\n", (int)currentchar, error);
			return 0;
		}

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0
		if((error = FTC_SBit_Cache_Lookup(cache, &desc, glyphindex, &sbit)))
#else
		FTC_Node anode;
		if((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
#endif
		{
			printf("<FTC_SBitCache_Lookup for Char \"%c\" failed with Errorcode 0x%.2X>\n", (int)currentchar, error);
			return 0;
		}

		if(use_kerning)
		{
			FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);

			prev_glyphindex = glyphindex;
			kerning.x >>= 6;
		}
		else kerning.x = 0;

	//render char

		if(color != -1) /* don't render char, return charwidth only */
		{
			if(sx + sbit->xadvance >= ex) return -1; /* limit to maxwidth */

			for(row = 0; row < sbit->height; row++)
			{
				for(pitch = 0; pitch < sbit->pitch; pitch++)
				{
					for(bit = 7; bit >= 0; bit--)
					{
						if(pitch*8 + 7-bit >= sbit->width) break; /* render needed bits only */

						if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) *(lbb + startx + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(starty + sy - sbit->top + y)) = color;

						x++;
					}
				}

				x = 0;
				y++;
			}
		}

	//return charwidth

		return sbit->xadvance + kerning.x;
}

/******************************************************************************
 * GetStringLen
 ******************************************************************************/

int GetStringLen(int sx, unsigned char *string)
{
int i, found;
int stringlen = 0;

	//reset kerning

		prev_glyphindex = 0;

	//calc len

		while(*string != '\0')
		{
			if(*string != '~')
			{
				stringlen += RenderChar(*string, -1, -1, -1, -1);
			}
			else
			{
				string++;
				if(*string=='t')
				{
#ifdef FT_NEW_CACHE_API
					stringlen=desc.width+TABULATOR*((int)(stringlen/TABULATOR)+1);
#else
					stringlen=desc.font.pix_width+TABULATOR*((int)(stringlen/TABULATOR)+1);
#endif
				}
				else
				{
					if(*string=='T')
					{
						if(sscanf(string+1,"%3d",&i)==1)
						{
							string+=3;
							stringlen=i-sx;
						}
						else
						{
#ifdef FT_NEW_CACHE_API
							stringlen=desc.width+TABULATOR*((int)(stringlen/TABULATOR)+1);
#else
							stringlen=desc.font.pix_width+TABULATOR*((int)(stringlen/TABULATOR)+1);
#endif
						}
					}
					else
					{
						found=0;
						for(i=0; i<sizeof(sc) && !found; i++)
						{
							if(*string==sc[i])
							{
								stringlen += RenderChar(tc[i], -1, -1, -1, -1);
								found=1;
							}
						}
					}
				}
			}				
			string++;
		}

	return stringlen;
}

/******************************************************************************
 * RenderString
 ******************************************************************************/

void RenderString(char *string, int sx, int sy, int maxwidth, int layout, int size, int color)
{
	int stringlen, ex, charwidth, i;
	char rstr[256], *rptr=rstr;
	int varcolor=color;

		strcpy(rstr,string);

	//set size

		switch (size)
		{
#ifdef FT_NEW_CACHE_API
			case SMALL: desc.width = desc.height = FSIZE_SMALL; break;
			case MED:   desc.width = desc.height = FSIZE_MED; break;
			case BIG:   desc.width = desc.height = FSIZE_BIG; break;
			default:    desc.width = desc.height = size; break;
#else
			case SMALL: desc.font.pix_width = desc.font.pix_height = FSIZE_SMALL; break;
			case MED:   desc.font.pix_width = desc.font.pix_height = FSIZE_MED; break;
			case BIG:   desc.font.pix_width = desc.font.pix_height = FSIZE_BIG; break;
			default:    desc.font.pix_width = desc.font.pix_height = size; break;
#endif
		}
		
	//set alignment

		if(layout != LEFT)
		{
			stringlen = GetStringLen(sx, string);

			switch(layout)
			{
				case CENTER:	if(stringlen < maxwidth) sx += (maxwidth - stringlen)/2;
						break;

				case RIGHT:	if(stringlen < maxwidth) sx += maxwidth - stringlen;
			}
		}

	//reset kerning

		prev_glyphindex = 0;

	//render string

		ex = sx + maxwidth;

		while(*rptr != '\0')
		{
			if(*rptr=='~')
			{
				++rptr;
				switch(*rptr)
				{
					case 'R': varcolor=RED; break;
					case 'G': varcolor=GREEN; break;
					case 'Y': varcolor=YELLOW; break;
					case 'B': varcolor=BLUE1; break;
					case 'S': varcolor=color; break;
					case 't': sx=((sx/TABULATOR)+1)*TABULATOR; break;
					case 'T': 
						if(sscanf(rptr+1,"%3d",&i)==1)
						{
							rptr+=3;
							sx=i;
						}
						else
						{
							sx=((sx/TABULATOR)+1)*TABULATOR;
						}
				}
			}
			else
			{
				if((charwidth = RenderChar(*rptr, sx, sy, ex, ((color!=CMCIT) && (color!=CMCST))?varcolor:color)) == -1) return; /* string > maxwidth */
				sx += charwidth;
			}
			rptr++;
		}
}

/******************************************************************************
 * ShowMessage
 ******************************************************************************/

void remove_tabs(char *src)
{
int i;
char *rmptr, *rmstr, *rmdptr;

	if(src && *src)
	{
		rmstr=strdup(src);
		rmdptr=rmstr;
		rmptr=src;
		while(*rmptr)
		{
			if(*rmptr=='~')
			{
				++rmptr;
				if(*rmptr)
				{
					if(*rmptr=='t')
					{
						*(rmdptr++)=' ';
					}
					else
					{
						if(*rmptr=='T')
						{
							*(rmdptr++)=' ';
							i=3;
							while(i-- && *(rmptr++));
						}
					}
					++rmptr;
				}
			}
			else
			{
				*(rmdptr++)=*(rmptr++);
			}
		}
		*rmdptr=0;
		strcpy(src,rmstr);
		free(rmstr);
	}
}

void ShowMessage(char *mtitle, char *message, int wait)
{
	extern int radius;
	int ixw=400;
	int lx=startx/*, ly=starty*/;
	char *tdptr;
	
	startx = sx + (((ex-sx) - ixw)/2);
//	starty=sy;
	
	//layout

		RenderBox(0, 178, ixw, 327, radius, CMH);
		RenderBox(2, 180, ixw-4, 323, radius, CMC);
		RenderBox(0, 178, ixw, 220, radius, CMH);

	//message
		
		tdptr=strdup(mtitle);
		remove_tabs(tdptr);
		RenderString(tdptr, 2, 213, ixw, CENTER, FSIZE_MED, CMHT);
		free(tdptr);
		tdptr=strdup(message);
		remove_tabs(tdptr);
		RenderString(tdptr, 2, 270, ixw, CENTER, FSIZE_MED, CMCT);
		free(tdptr);

		if(wait)
		{
			RenderBox(ixw/2-25, 286, ixw/2+25, 310, radius, CMCS);
			RenderString("OK", ixw/2-25, 305, 50, CENTER, SMALL, CMCT);
		}
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

		while(wait && (GetRCCode() != RC_OK));
		
		startx=lx;
//		starty=ly;

}

