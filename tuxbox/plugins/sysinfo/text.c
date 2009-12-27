/*
 * $Id: text.c,v 1.2 2009/12/27 12:08:02 rhabarber1848 Exp $
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

#include "text.h"
#include "gfx.h"
#include "io.h"

int FSIZE_BIG=40;
int FSIZE_MED=30;
int FSIZE_SMALL=24;

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

int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, int color, int size)
{
	int row, pitch, bit, x = 0, y = 0;
	FT_UInt glyphindex;
	FT_Vector kerning;
	FTC_Node anode;
#ifdef FT_NEW_CACHE_API
	if (size !=0) desc.width = desc.height = size; 
#else
	if (size !=0) desc.font.pix_width = desc.font.pix_height = size; 
#endif

	//load char

		if(!(glyphindex = FT_Get_Char_Index(face, currentchar)))
		{
			printf("<FT_Get_Char_Index for Char \"%c\" failed with Errorcode 0x%.2X>\n", (int)currentchar, error);
			return 0;
		}

		if((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
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

//						if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) *(lbb + startx + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(starty + sy - sbit->top + y)) = color;
						if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit) *(lbb + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(sy - sbit->top + y)) = color;

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

int GetStringLen(unsigned char *string)
{
	int stringlen = 0;

	//reset kerning

		prev_glyphindex = 0;

	//calc len

		while(*string != '\0')
		{
			stringlen += RenderChar(*string, -1, -1, -1, -1,0);
			string++;
		}

	return stringlen;
}

/******************************************************************************
 * RenderString
 ******************************************************************************/

void RenderString(char *string, int sx, int sy, int maxwidth, int layout, int size, int color)
{
	int stringlen, ex, charwidth;

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
			stringlen = GetStringLen(string);

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

		while(*string != '\0')
		{
			if((charwidth = RenderChar(*string, sx, sy, ex, color, 0)) == -1) return; /* string > maxwidth */

			sx += charwidth;
			string++;
		}
}
