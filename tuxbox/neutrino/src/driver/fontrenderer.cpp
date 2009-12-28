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

/* tested with freetype 2.3.9, and 2.1.4 */
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
#define FT_NEW_CACHE_API
#endif

#include <driver/fontrenderer.h>

#include <system/debug.h>


FT_Error FBFontRenderClass::myFTC_Face_Requester(FTC_FaceID  face_id,
        FT_Library  /*library*/,
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
	if (FTC_SBitCache_New(cacheManager, &sbitsCache))
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
	fontListEntry *fnt=(fontListEntry *)face_id;
	if (!fnt)
		return -1;
	dprintf(DEBUG_DEBUG, "[FONT] FTC_Face_Requester (%s/%s)\n", fnt->family, fnt->style);

	int error;
	if ((error=FT_New_Face(library, fnt->filename, 0, aface)))
	{
		dprintf(DEBUG_NORMAL, "[FONT] FTC_Face_Requester (%s/%s) failed: %i\n", fnt->family, fnt->style, error);
		return error;
	}

	if (strcmp(fnt->style, (*aface)->style_name) != 0)
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

#ifdef FT_NEW_CACHE_API
FT_Error FBFontRenderClass::getGlyphBitmap(FTC_ImageType fnt, FT_ULong glyph_index, FTC_SBit *sbit)
{
	return FTC_SBitCache_Lookup(sbitsCache, fnt, glyph_index, sbit, NULL);
}
#else
FT_Error FBFontRenderClass::getGlyphBitmap(FTC_Image_Desc *fnt, FT_ULong glyph_index, FTC_SBit *sbit)
{
	return FTC_SBit_Cache_Lookup(sbitsCache, fnt, glyph_index, sbit);
}
#endif

const char *FBFontRenderClass::AddFont(const char * const filename, const bool make_italics)
{
	fflush(stdout);
	int error;
	fontListEntry *n=new fontListEntry;

	FT_Face face;
	if ((error=FT_New_Face(library, filename, 0, &face)))
	{
		dprintf(DEBUG_NORMAL, "[FONT] adding font %s, failed: %i\n", filename, error);
		delete n;
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
	free(filename);
	free(family);
	free(style);
}

Font *FBFontRenderClass::getFont(const char * const family, const char * const style, int size)
{
	FTC_FaceID id = getFaceID(family, style);
	if (!id)
		return 0;
	return new Font(this, id, size, (strcmp(((fontListEntry *)id)->style, style) == 0) ? Font::Regular : Font::Embolden);
}

std::string FBFontRenderClass::getFamily(const char * const filename) const
{
	for (fontListEntry *f=font; f; f=f->next)
	{
		if (!strcmp(f->filename, filename))
			return f->family;
	}

  return "";
}

Font::Font(FBFontRenderClass *render, FTC_FaceID faceid, const int isize, const fontmodifier _stylemodifier)
{
	stylemodifier           = _stylemodifier;

	frameBuffer 		= CFrameBuffer::getInstance();
	renderer 		= render;
#ifdef FT_NEW_CACHE_API
	font.face_id	 	= faceid;
	font.width  		= isize;
	font.height	 	= isize;
	font.flags	 	= FT_LOAD_FORCE_AUTOHINT;
#else
	font.font.face_id 	= faceid;
	font.font.pix_width  	= isize;
	font.font.pix_height 	= isize;
	font.image_type 	= ftc_image_grays;
	font.image_type 	|= ftc_image_flag_autohinted;
#endif
	setSize(isize);
}

FT_Error Font::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

int Font::setSize(int isize)
{
	FT_Error err;
#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;

	int temp = font.width;
	font.width = font.height = isize;
	scaler.face_id = font.face_id;
	scaler.width   = font.width;
	scaler.height  = font.height;
	scaler.pixel   = true;

	err = FTC_Manager_LookupSize(renderer->cacheManager, &scaler, &size);
#else
	int temp = font.font.pix_width; 
	font.font.pix_width = font.font.pix_height = isize; 

	err = FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size);
#endif
	if (err != 0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed! (0x%x)\n", err);
		return 0;
	}
	// hack begin (this is a hack to get correct font metrics, didn't find any other way which gave correct values)
	FTC_SBit glyph;
	int index;

#ifdef FT_NEW_CACHE_API
	index = FT_Get_Char_Index(size->face, 'M');
#else
	index = FT_Get_Char_Index(face, 'M'); // "M" gives us ascender
#endif
	getGlyphBitmap(index, &glyph);
	int tM=glyph->top;
	fontwidth = glyph->width;

#ifdef FT_NEW_CACHE_API
	index = FT_Get_Char_Index(size->face, 'g');
#else
	index = FT_Get_Char_Index(face, 'g'); // "g" gives us descender
#endif
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

	FT_Error err;

	pthread_mutex_lock( &renderer->render_mutex );
#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;

	scaler.face_id = font.face_id;
	scaler.width   = font.width;
	scaler.height  = font.height;
	scaler.pixel   = true;

	err = FTC_Manager_LookupSize(renderer->cacheManager, &scaler, &size);
#else
	err = FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size);
#endif
	if (err != 0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed! (0x%x)\n", err);
		pthread_mutex_unlock(&renderer->render_mutex);
		return;
	}
#ifdef FT_NEW_CACHE_API
	int use_kerning = FT_HAS_KERNING(size->face);
#else
	int use_kerning = FT_HAS_KERNING(face);
#endif
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
	fb_pixel_t colors[256];

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
	t_fb_var_screeninfo * screeninfo = frameBuffer->getScreenInfo();
        int rl = screeninfo->red.length;
        int ro = screeninfo->red.offset;
        int gl = screeninfo->green.length;
        int go = screeninfo->green.offset;
        int bl = screeninfo->blue.length;
        int bo = screeninfo->blue.offset;
        int tl = screeninfo->transp.length;
        int to = screeninfo->transp.offset;

	fb_pixel_t bgcolor = frameBuffer->realcolor[color];
	fb_pixel_t fgcolor = frameBuffer->realcolor[(((((int)color) + 2) | 7) - 2)];
	int fgr = (((int)fgcolor >> ro) & ((1 << rl) - 1));
	int fgg = (((int)fgcolor >> go) & ((1 << gl) - 1));
	int fgb = (((int)fgcolor >> bo) & ((1 << bl) - 1));
	int fgt = (((int)fgcolor >> to) & ((1 << tl) - 1));
	int deltar = (((int)bgcolor >> ro) & ((1 << rl) - 1)) - fgr;
	int deltag = (((int)bgcolor >> go) & ((1 << gl) - 1)) - fgg;
	int deltab = (((int)bgcolor >> bo) & ((1 << bl) - 1)) - fgb;
	int deltat = (((int)bgcolor >> to) & ((1 << tl) - 1)) - fgt;

	fb_pixel_t colors[256];

	for (int i = 0; i < 256; i++)
	{
		colors[255 - i] =
			((((fgr + deltar * i / 255) & ((1 << rl) - 1)) << ro) |
			 (((fgg + deltag * i / 255) & ((1 << gl) - 1)) << go) |
			 (((fgb + deltab * i / 255) & ((1 << bl) - 1)) << bo) |
			 (((fgt + deltat * i / 255) & ((1 << tl) - 1)) << to));
	}
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

		int index = FT_Get_Char_Index(size->face, unicode_value);

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
			FT_Get_Kerning(size->face, lastindex, index, 0, &kerning);
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
					*td++= colors[*s++];
#endif
				}
				else
				{
					int start, end;
					int col = -1;

					if (ax < w)
						start = 0;
					else
						start = ax - w + 1;

					if (ax < spread_by)
						end = ax + 1;
					else
						end = spread_by + 1;

					for (int i = start; i < end; i++)
						if (col < *(s - i))
							col = *(s - i);
#ifdef FB_USE_PALETTE
#ifdef PRE_CALC_TRANSLATION_TABLE
					*td++= colors[col];
#else
					*(td++) = bgcolor + (fb_pixel_t)(col * delta / 256);
#endif
#else
					*td++= colors[col];
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
	FT_Error err;
#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;

	scaler.face_id = font.face_id;
	scaler.width   = font.width;
	scaler.height  = font.height;
	scaler.pixel   = true;

	err = FTC_Manager_LookupSize(renderer->cacheManager, &scaler, &size);
#else
	err = FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size);
#endif
	if (err != 0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed! (0x%x)\n", err);
		pthread_mutex_unlock(&renderer->render_mutex);
		return -1;
	}
#ifdef FT_NEW_CACHE_API
	int use_kerning = FT_HAS_KERNING(size->face);
#else
	int use_kerning = FT_HAS_KERNING(face);
#endif

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

		int index = FT_Get_Char_Index(size->face, unicode_value);

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
			FT_Get_Kerning(size->face, lastindex, index, 0, &kerning);
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
