/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
        Copyright (C) 2003 thegoodguy

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
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

// this method is recommended for FreeType >2.0.x:
#include <ft2build.h>
#include FT_FREETYPE_H

#include <driver/fontrenderer.h>

#include <system/debug.h>


FT_Error FBFontRenderClass::myFTC_Face_Requester(FTC_FaceID  face_id,
        FT_Library  library,
        FT_Pointer  request_data,
        FT_Face*    aface)
{
	return ((FBFontRenderClass*)request_data)->FTC_Face_Requester(face_id, aface);
}


FBFontRenderClass::FBFontRenderClass()
{
	dprintf(DEBUG_DEBUG, "[FONT] initializing core...\n");
	if (FT_Init_FreeType(&library))
	{
		dprintf(DEBUG_NORMAL, "[FONT] initializing core failed.\n");
		return;
	}

	font = NULL;

	int maxbytes= 4 *1024*1024;
	dprintf(DEBUG_INFO, "[FONT] Intializing font cache, using max. %dMB...\n", maxbytes/1024/1024);
	fflush(stdout);
	if (FTC_Manager_New(library, 10, 20, maxbytes, myFTC_Face_Requester, this, &cacheManager))
	{
		dprintf(DEBUG_NORMAL, "[FONT] manager failed!\n");
		return;
	}
	if (!cacheManager)
	{
		dprintf(DEBUG_NORMAL, "[FONT] error.\n");
		return;
	}
	if (FTC_SBit_Cache_New(cacheManager, &sbitsCache))
	{
		dprintf(DEBUG_NORMAL, "[FONT] sbit failed!\n");
		return;
	}
/*	if (FTC_ImageCache_New(cacheManager, &imageCache))
	{
		printf(" imagecache failed!\n");
	}
*/
	pthread_mutex_init( &render_mutex, NULL );
}

FBFontRenderClass::~FBFontRenderClass()
{
	fontListEntry * g;
	
	for (fontListEntry * f = font; f; f = g)
	{
		g = f->next;
		delete f;
	}

	FTC_Manager_Done(cacheManager);
	FT_Done_FreeType(library);
}

FT_Error FBFontRenderClass::FTC_Face_Requester(FTC_FaceID face_id, FT_Face* aface)
{
	fontListEntry *font=(fontListEntry *)face_id;
	if (!font)
		return -1;
	dprintf(DEBUG_DEBUG, "[FONT] FTC_Face_Requester (%s/%s)\n", font->family, font->style);

	int error;
	if ((error=FT_New_Face(library, font->filename, 0, aface)))
	{
		dprintf(DEBUG_NORMAL, "[FONT] FTC_Face_Requester (%s/%s) failed: %i\n", font->family, font->style, error);
		return error;
	}

	if (strcmp(font->style, (*aface)->style_name) != 0)
	{
		FT_Matrix matrix; // Italics

		matrix.xx = 1 * 0x10000;
		matrix.xy = (0x10000 >> 3);
		matrix.yx = 0 * 0x10000;
		matrix.yy = 1 * 0x10000;

		FT_Set_Transform(*aface, &matrix, NULL);
	}
	return 0;
}

FTC_FaceID FBFontRenderClass::getFaceID(const char * const family, const char * const style)
{
	for (fontListEntry *f=font; f; f=f->next)
	{
		if ((!strcmp(f->family, family)) && (!strcmp(f->style, style)))
			return (FTC_FaceID)f;
	}
	if (strncmp(style, "Bold ", 5) == 0)
		for (fontListEntry *f=font; f; f=f->next)
		{
			if ((!strcmp(f->family, family)) && (!strcmp(f->style, &(style[5]))))
				return (FTC_FaceID)f;
		}
	return 0;
}

FT_Error FBFontRenderClass::getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	return FTC_SBit_Cache_Lookup(sbitsCache, font, glyph_index, sbit);
}

const char * const FBFontRenderClass::AddFont(const char * const filename, const bool make_italics)
{
	fflush(stdout);
	int error;
	fontListEntry *n=new fontListEntry;

	FT_Face face;
	if ((error=FT_New_Face(library, filename, 0, &face)))
	{
		dprintf(DEBUG_NORMAL, "[FONT] adding font %s, failed: %i\n", filename, error);
		return NULL;
	}
	n->filename = strdup(filename);
	n->family   = strdup(face->family_name);
	n->style    = strdup(make_italics ? "Italic" : face->style_name);
	FT_Done_Face(face);
	n->next=font;
	dprintf(DEBUG_DEBUG, "[FONT] adding font %s... ok\n", filename);
	font=n;
	return n->style;
}

FBFontRenderClass::fontListEntry::~fontListEntry()
{
	delete[] filename;
	delete[] family;
	delete[] style;
}

Font *FBFontRenderClass::getFont(const char * const family, const char * const style, int size)
{
	FTC_FaceID id = getFaceID(family, style);
	if (!id)
		return 0;
	return new Font(this, id, size, (strcmp(((fontListEntry *)id)->style, style) == 0) ? Font::Regular : Font::Embolden);
}

Font::Font(FBFontRenderClass *render, FTC_FaceID faceid, const int isize, const fontmodifier _stylemodifier)
{
	stylemodifier           = _stylemodifier;

	frameBuffer 		= CFrameBuffer::getInstance();
	renderer 		= render;
	font.font.face_id 	= faceid;
	font.font.pix_width  	= isize;
	font.font.pix_height 	= isize;
	font.image_type 	= ftc_image_grays;
	font.image_type 	|= ftc_image_flag_autohinted;

	setSize(isize);
}

FT_Error Font::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

int Font::setSize(int isize)
{
	int temp = font.font.pix_width; 
	font.font.pix_width = font.font.pix_height = isize; 

	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed!\n");
		return 0;
	}
	// hack begin (this is a hack to get correct font metrics, didn't find any other way which gave correct values)
	FTC_SBit glyph;
	int index;

	index=FT_Get_Char_Index(face, 'M'); // "M" gives us ascender
	getGlyphBitmap(index, &glyph);
	int tM=glyph->top;
	fontwidth = glyph->width;

	index=FT_Get_Char_Index(face, 'g'); // "g" gives us descender
	getGlyphBitmap(index, &glyph);
	int hg=glyph->height;
	int tg=glyph->top;

	ascender=tM;
	descender=tg-hg; //this is a negative value!
	int halflinegap= -(descender>>1); // |descender/2| - we use descender as linegap, half at top, half at bottom
	upper = halflinegap+ascender+3;   // we add 3 at top
	lower = -descender+halflinegap+1; // we add 1 at bottom
	height=upper+lower;               // this is total height == distance of lines
	// hack end

	//printf("glyph: hM=%d tM=%d hg=%d tg=%d ascender=%d descender=%d height=%d linegap/2=%d upper=%d lower=%d\n",
	//       hM,tM,hg,tg,ascender,descender,height,halflinegap,upper,lower);
	//printf("font metrics: height=%ld\n", (size->metrics.height+32) >> 6);
	return temp;
}

int Font::getHeight(void)
{
	return height;
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

void Font::RenderString(int x, int y, const int width, const char *text, const unsigned char color, const int boxheight, const bool utf8_encoded)
{
	if (!frameBuffer->getActive())
		return;

	pthread_mutex_lock( &renderer->render_mutex );

	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed!\n");
		return;
	}

	int use_kerning=FT_HAS_KERNING(face);

	int left=x;
	int step_y=height;

	// ----------------------------------- box upper end (this is NOT a font metric, this is our method for y centering)
	//
	// **  -------------------------------- y=baseline-upper
	// ||
	// |u  --------------------*----------- y=baseline+ascender
	// |p                     * *
	// hp                    *   *
	// ee     *        *    *     *
	// ir      *      *     *******
	// g|       *    *      *     *
	// h|        *  *       *     *
	// t*  -------**--------*-----*-------- y=baseline
	// |l         *
	// |o        *
	// |w  -----**------------------------- y=baseline+descender   // descender is a NEGATIVE value
	// |r
	// **  -------------------------------- y=baseline+lower == YCALLER
	//
	// ----------------------------------- box lower end (this is NOT a font metric, this is our method for y centering)

	// height = ascender + -1*descender + linegap           // descender is negative!

	// now we adjust the given y value (which is the line marked as YCALLER) to be the baseline after adjustment:
	y -= lower;
	// y coordinate now gives font baseline which is used for drawing

	// caution: this only works if we print a single line of text
	// if we have multiple lines, don't use boxheight or specify boxheight==0.
	// if boxheight is !=0, we further adjust y, so that text is y-centered in the box
	if(boxheight)
	{
		if(boxheight>step_y)			// this is a real box (bigger than text)
			y -= ((boxheight-step_y)>>1);
		else if(boxheight<0)			// this normally would be wrong, so we just use it to define a "border"
			y += (boxheight>>1);		// half of border value at lower end, half at upper end
	}

	int lastindex=0; // 0 == missing glyph (never has kerning values)
	FT_Vector kerning;
	int pen1=-1; // "pen" positions for kerning, pen2 is "x"

#ifdef FB_USE_PALETTE
#define PRE_CALC_TRANSLATION_TABLE
#ifdef PRE_CALC_TRANSLATION_TABLE
	unsigned char colors[256];

	int coff = 7 - ((color + 2) & 7);
	for (int i = (256 - 32); i >= 0; i -= 32)
	{
		memset(&(colors[i]), coff + color, 32);

		if (coff != 0)
			coff--;
	}
#else
	fb_pixel_t bgcolor = color;
	int fgcolor = (((((int)color) + 2) | 7) - 2) + 1;
	int delta = fgcolor - bgcolor;
#endif
#else
	fb_pixel_t bgcolor = frameBuffer->realcolor[color];
	int fgcolor = frameBuffer->realcolor[(((((int)color) + 2) | 7) - 2)];
	int delta = fgcolor - bgcolor;
#endif 
	
	int spread_by = 0;
	if (stylemodifier == Font::Embolden)
	{
		spread_by = (fontwidth / 6) - 1;
		if (spread_by < 1)
			spread_by = 1;
	}

	for (; *text; text++)
	{
		FTC_SBit glyph;
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
			dprintf(DEBUG_NORMAL, "failed to get glyph bitmap.\n");
			continue;
		}

		// width clip
		if (x + glyph->xadvance + spread_by > left + width)
			break;

		//kerning
		if(use_kerning)
		{
			FT_Get_Kerning(face,lastindex,index,0,&kerning);
			x+=(kerning.x+32)>>6; // kerning!
		}

		int stride  = frameBuffer->getStride();
		uint8_t * d = ((uint8_t *)frameBuffer->getFrameBufferPointer()) + (x + glyph->left) * sizeof(fb_pixel_t) + stride * (y - glyph->top);
		uint8_t * s = glyph->buffer;
		int w       = glyph->width;
		int h       = glyph->height;
		int pitch   = glyph->pitch;

		for (int ay=0; ay<h; ay++)
		{
			fb_pixel_t * td = (fb_pixel_t *)d;

			int ax;
			for (ax=0; ax < w + spread_by; ax++)
			{
				if (stylemodifier != Font::Embolden)
				{			
#ifdef FB_USE_PALETTE
#ifdef PRE_CALC_TRANSLATION_TABLE
					*td++= colors[*s++];
#else
					*(td++) = bgcolor + (fb_pixel_t)(((int)(*s++)) * delta / 256);
#endif
#else
					*(td++) = bgcolor + (fb_pixel_t)(((int)(*s++)) * delta / 256);
#endif
				}
				else
				{
					int start, end;
					int color = -1;

					if (ax < w)
						start = 0;
					else
						start = ax - w + 1;

					if (ax < spread_by)
						end = ax + 1;
					else
						end = spread_by + 1;

					for (int i = start; i < end; i++)
						if (color < *(s - i))
							color = *(s - i);
#ifdef FB_USE_PALETTE
#ifdef PRE_CALC_TRANSLATION_TABLE
					*td++= colors[color];
#else
					*(td++) = bgcolor + (fb_pixel_t)(color * delta / 256);
#endif
#else
					*(td++) = bgcolor + (fb_pixel_t)(color * delta / 256);
#endif
					s++;
				}
			}
			s += pitch- ax;
			d += stride;
		}
		x+=glyph->xadvance+1;
		if(pen1>x)
			x=pen1;
		pen1=x;
		lastindex=index;
	}
	//printf("RenderStat: %d %d %d \n", renderer->cacheManager->num_nodes, renderer->cacheManager->num_bytes, renderer->cacheManager->max_bytes);
	pthread_mutex_unlock( &renderer->render_mutex );
}

void Font::RenderString(int x, int y, const int width, const std::string & text, const unsigned char color, const int boxheight, const bool utf8_encoded)
{
	RenderString(x, y, width, text.c_str(), color, boxheight, utf8_encoded);
}

int Font::getRenderWidth(const char *text, const bool utf8_encoded)
{
	pthread_mutex_lock( &renderer->render_mutex );

	int use_kerning=FT_HAS_KERNING(face);
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed!\n");
		return -1;
	}

	int x=0;
	int lastindex=0; // 0==missing glyph (never has kerning)
	FT_Vector kerning;
	int pen1=-1; // "pen" positions for kerning, pen2 is "x"
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
			dprintf(DEBUG_NORMAL, "failed to get glyph bitmap.\n");
			continue;
		}
		//kerning
		if(use_kerning)
		{
			FT_Get_Kerning(face,lastindex,index,0,&kerning);
			x+=(kerning.x+32)>>6; // kerning!
		}

		x+=glyph->xadvance+1;
		if(pen1>x)
			x=pen1;
		pen1=x;
		lastindex=index;
	}

	if (stylemodifier == Font::Embolden)
	{
		int spread_by = (fontwidth / 6) - 1;
		if (spread_by < 1)
			spread_by = 1;

		x += spread_by;
	}

	pthread_mutex_unlock( &renderer->render_mutex );

	return x;
}

int Font::getRenderWidth(const std::string & text, const bool utf8_encoded)
{
	return getRenderWidth(text.c_str(), utf8_encoded);
}
