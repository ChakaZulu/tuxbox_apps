/*
        $Header: /cvs/tuxbox/apps/tuxbox/libs/liblcddisplay/fontrenderer.cpp,v 1.11 2009/02/24 19:09:06 seife Exp $        

	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2003 thegoodguy
		baseroutines by tmbinc


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

#include <config.h>

#include "fontrenderer.h"

#include <stdio.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Error LcdFontRenderClass::myFTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Library  library,
                            FT_Pointer  request_data,
                            FT_Face*    aface)
{
	return ((LcdFontRenderClass*)request_data)->FTC_Face_Requester(face_id, aface);
}


LcdFontRenderClass::LcdFontRenderClass(CLCDDisplay * fb)
{
	framebuffer = fb;
	printf("[LCDFONT] initializing core...");
	fflush(stdout);
	if (FT_Init_FreeType(&library))
	{
		printf("failed.\n");
		return;
	}
	printf("\n");
	font=0;
}

LcdFontRenderClass::~LcdFontRenderClass()
{
	FTC_Manager_Done(cacheManager);
	FT_Done_FreeType(library);
}

void LcdFontRenderClass::InitFontCache()
{
	printf("[LCDFONT] Intializing font cache...");
	fflush(stdout);
	if (FTC_Manager_New(library, 3, 0, 0, myFTC_Face_Requester, this, &cacheManager))
	{
		printf(" manager failed!\n");
		return;
	}
	if (!cacheManager)
	{
		printf(" error.\n");
		return;
	}
	if (FTC_SBit_Cache_New(cacheManager, &sbitsCache))
	{
		printf(" sbit failed!\n");
		return;
	}
	if (FTC_Image_Cache_New(cacheManager, &imageCache))
	{
		printf(" imagecache failed!\n");
	}
	printf("\n");
}

FT_Error LcdFontRenderClass::FTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Face*    aface)
{
	fontListEntry *font=(fontListEntry *)face_id;
	if (!font)
		return -1;
	printf("[LCDFONT] FTC_Face_Requester (%s/%s)\n", font->family, font->style);

	int error;
	if ((error=FT_New_Face(library, font->filename, 0, aface)))
	{
		printf(" failed: %i\n", error);
		return error;
	}
	return 0;
}                                                                                                                                

FTC_FaceID LcdFontRenderClass::getFaceID(const char *family, const char *style)
{
	for (fontListEntry *f=font; f; f=f->next)
	{
		if ((!strcmp(f->family, family)) && (!strcmp(f->style, style)))
			return (FTC_FaceID)f;
	}
	return 0;
}

FT_Error LcdFontRenderClass::getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	return FTC_SBit_Cache_Lookup(sbitsCache, font, glyph_index, sbit);
}

const char * LcdFontRenderClass::AddFont(const char * const filename)
{
	printf("[LCDFONT] adding font %s...", filename);
	fflush(stdout);
	int error;
	fontListEntry *n=new fontListEntry;

	FT_Face face;
	if ((error=FT_New_Face(library, filename, 0, &face)))
	{
		printf(" failed: %i\n", error);
		return NULL;
	}
	n->filename = strdup(filename);
	n->family   = strdup(face->family_name);
	n->style    = strdup(face->style_name);
	FT_Done_Face(face);

	n->next=font;
	printf("OK (%s/%s)\n", n->family, n->style);
	font=n;
	return n->style;
}

LcdFontRenderClass::fontListEntry::~fontListEntry()
{
	delete[] filename;
	delete[] family;
	delete[] style;
}

LcdFont *LcdFontRenderClass::getFont(const char *family, const char *style, int size)
{
	FTC_FaceID id=getFaceID(family, style);
	if (!id)
		return 0;
	return new LcdFont(framebuffer, this, id, size);
}

LcdFont::LcdFont(CLCDDisplay * fb, LcdFontRenderClass *render, FTC_FaceID faceid, int isize)
{
	framebuffer=fb;
	renderer=render;
	font.font.face_id=faceid;
	font.font.pix_width  = isize;
	font.font.pix_height = isize;
	font.image_type = ftc_image_mono;
	font.image_type |= ftc_image_flag_autohinted;
}

FT_Error LcdFont::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

int UTF8ToUnicode(const char * &text, const bool utf8_encoded) // returns -1 on error
{
	int unicode_value;
	
	if (utf8_encoded && ((((unsigned char)(*text)) & 0x80) != 0))
	{
		int remaining_unicode_length;
		if ((((unsigned char)(*text)) & 0xf8) == 0xf0)
		{
			unicode_value = ((unsigned char)(*text)) & 0x07;
			remaining_unicode_length = 3;
		}
		else if ((((unsigned char)(*text)) & 0xf0) == 0xe0)
		{
			unicode_value = ((unsigned char)(*text)) & 0x0f;
			remaining_unicode_length = 2;
		}
		else if ((((unsigned char)(*text)) & 0xe0) == 0xc0)
		{
			unicode_value = ((unsigned char)(*text)) & 0x1f;
			remaining_unicode_length = 1;
		}
		else                     // cf.: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
			return -1;       // corrupted character or a character with > 4 bytes utf-8 representation
		
		for (int i = 0; i < remaining_unicode_length; i++)
		{
			text++;
			if (((*text) & 0xc0) != 0x80)
			{
				remaining_unicode_length = -1;
				return -1;          // incomplete or corrupted character
			}
			unicode_value <<= 6;
			unicode_value |= ((unsigned char)(*text)) & 0x3f;
		}
		if (remaining_unicode_length == -1)
			return -1;                  // incomplete or corrupted character
	}
	else
		unicode_value = (unsigned char)(*text);

	return unicode_value;
}

void LcdFont::RenderString(int x, int y, const int width, const char * text, const int color, const int selected, const bool utf8_encoded)
{
	int err;
	if ((err=FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size))!=0)
	{ 
		printf("FTC_Manager_Lookup_Size failed! (%d)\n",err);
		return;
	}
	int left=x, step_y=(size->metrics.height >> 6 )*3/4 + 4;

	int pos =0;
	for (; *text; text++)
	{
		pos++;
		FTC_SBit glyph;
		//if ((x + size->metrics.x_ppem > (left+width)) || (*text=='\n'))
		if (x + size->metrics.x_ppem > (left+width))
		{ //width clip
			return;
		}
		if (*text=='\n')
		{
		  x  = left;
		  y += step_y;
		}

		int unicode_value = UTF8ToUnicode(text, utf8_encoded);

		if (unicode_value == -1)
			break;

		int index = FT_Get_Char_Index(face, unicode_value);

		if (!index)
		  continue;
		if (getGlyphBitmap(index, &glyph))
		{
		  printf("failed to get glyph bitmap.\n");
		  continue;
		}
    
		int rx=x+glyph->left;
		int ry=y-glyph->top;
		if(pos==selected)
		{
			framebuffer->draw_fill_rect(x-2,y-glyph->height-2, x+glyph->width+2, y+2, CLCDDisplay::PIXEL_INV );
		}
		
		for (int ay=0; ay<glyph->height; ay++)
		{
			int ax=0;
			int w=glyph->width;
			int xpos = rx;
			for (; ax<w; ax++)
			{
				unsigned char c = glyph->buffer[ay*abs(glyph->pitch)+(ax>>3)];
				if((c>>(7-(ax&7)))&1)
				framebuffer->draw_point(xpos,ry, color);
				xpos ++;
			}
		ry++;
		}

    x+=glyph->xadvance+1;
    }
}

int LcdFont::getRenderWidth(const char * text, const bool utf8_encoded)
{
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return -1;
	}
	int x=0;
	for (; *text; text++)
	{
		FTC_SBit glyph;

		int unicode_value = UTF8ToUnicode(text, utf8_encoded);

		if (unicode_value == -1)
			break;

		int index=FT_Get_Char_Index(face, unicode_value);

		if (!index)
			continue;
		if (getGlyphBitmap(index, &glyph))
		{
			printf("failed to get glyph bitmap.\n");
			continue;
		}
    
		x+=glyph->xadvance+1;
	}
	return x;
}

