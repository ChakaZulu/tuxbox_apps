/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
		baseroutines by tmbinc
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

#include <stdio.h>
#include <stdlib.h>
#include "fontrenderer.h"
#include <config.h>

#include <freetype/freetype.h>

  /* the following header shouldn't be used in normal programs */
#include <freetype/internal/ftdebug.h>

  /* showing driver name */
#include <freetype/ftmodule.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftdriver.h>

FT_Error fontRenderClass::myFTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Library  library,
                            FT_Pointer  request_data,
                            FT_Face*    aface)
{
	return ((fontRenderClass*)request_data)->FTC_Face_Requester(face_id, aface);
}


fontRenderClass::fontRenderClass(CLCDDisplay *fb)
{
	framebuffer = fb;
	printf("[FONT] initializing core...");
	fflush(stdout);
	if (FT_Init_FreeType(&library))
	{
		printf("failed.\n");
		return;
	}
	printf("\n");
	font=0;
}

fontRenderClass::~fontRenderClass()
{
	FTC_Manager_Done(cacheManager);
	FT_Done_FreeType(library);
}

void fontRenderClass::InitFontCache()
{
	printf("[FONT] Intializing font cache...");
	fflush(stdout);
	if (FTC_Manager_New(library, 0, 0, 0, myFTC_Face_Requester, this, &cacheManager))
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

FT_Error fontRenderClass::FTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Face*    aface)
{
	fontListEntry *font=(fontListEntry *)face_id;
	if (!font)
		return -1;
	printf("[FONT] FTC_Face_Requester (%s/%s)\n", font->family, font->style);

	int error;
	if ((error=FT_New_Face(library, font->filename, 0, aface)))
	{
		printf(" failed: %i\n", error);
		return error;
	}
	return 0;
}                                                                                                                                

FTC_FaceID fontRenderClass::getFaceID(const char *family, const char *style)
{
	for (fontListEntry *f=font; f; f=f->next)
	{
		if ((!strcmp(f->family, family)) && (!strcmp(f->style, style)))
			return (FTC_FaceID)f;
	}
	return 0;
}

FT_Error fontRenderClass::getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	return FTC_SBit_Cache_Lookup(sbitsCache, font, glyph_index, sbit);
}

int fontRenderClass::AddFont(const char *filename)
{
	printf("[FONT] adding font %s...", filename);
	fflush(stdout);
	int error;
	fontListEntry *n=new fontListEntry;

	FT_Face face;
	if ((error=FT_New_Face(library, filename, 0, &face)))
	{
		printf(" failed: %i\n", error);
		return error;
	}
	strcpy(n->filename=new char[strlen(filename)], filename);
	strcpy(n->family=new char[strlen(filename)], face->family_name);
	strcpy(n->style=new char[strlen(filename)], face->style_name);
	FT_Done_Face(face);

	n->next=font;
	printf("OK (%s/%s)\n", n->family, n->style);
	font=n;
	return 0;
}

fontRenderClass::fontListEntry::~fontListEntry()
{
	delete[] filename;
	delete[] family;
	delete[] style;
}

Font *fontRenderClass::getFont(const char *family, const char *style, int size)
{
	FTC_FaceID id=getFaceID(family, style);
	if (!id)
		return 0;
	return new Font(framebuffer, this, id, size);
}

Font::Font(CLCDDisplay *fb, fontRenderClass *render, FTC_FaceID faceid, int isize)
{
	framebuffer=fb;
	renderer=render;
	font.font.face_id=faceid;
	font.font.pix_width  = isize;
	font.font.pix_height = isize;
	font.image_type = ftc_image_mono;
	font.image_type |= ftc_image_flag_autohinted;
}

FT_Error Font::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

void Font::RenderString(int x, int y, int width, const char *string, int color, int selected)
{
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return;
	}
	int left=x, step_y=(size->metrics.height >> 6 )*3/4 + 4;

	int pos =0;
	for (; *string; string++)
	{
		pos++;
		FTC_SBit glyph;
		//if ((x + size->metrics.x_ppem > (left+width)) || (*string=='\n'))
		if (x + size->metrics.x_ppem > (left+width))
		{ //width clip
			return;
		}
		if (*string=='\n')
		{
		  x  = left;
		  y += step_y;
		}
		int index=FT_Get_Char_Index(face, *string);
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

int Font::getRenderWidth(const char *string)
{
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return -1;
	}
	int x=0;
	for (; *string; string++)
	{
		FTC_SBit glyph;

		int index=FT_Get_Char_Index(face, *string);
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

