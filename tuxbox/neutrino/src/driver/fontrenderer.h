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


#ifndef __FONTRENDERER__
#define __FONTRENDERER__

#include <pthread.h>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H

#include "framebuffer.h"


class FBFontRenderClass;
class Font
{
	CFrameBuffer	*frameBuffer;
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
	FTC_ImageTypeRec font;
#else
	FTC_Image_Desc	font;
	FT_Face		face;
#endif
	FBFontRenderClass *renderer;
	FT_Size		size;

	FT_Error getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit);

	// these are HACKED values, because the font metrics were unusable.
	int height,ascender,descender,upper,lower;
	int fontwidth;

 public:
	enum fontmodifier
		{
			Regular,
			Embolden
		};
	fontmodifier stylemodifier;

	void RenderString(int x, int y, const int width, const char *        text, const unsigned char color, const int boxheight = 0, const bool utf8_encoded = false);
	void RenderString(int x, int y, const int width, const std::string & text, const unsigned char color, const int boxheight = 0, const bool utf8_encoded = false);

	int getRenderWidth(const char *        text, const bool utf8_encoded = false);
	int getRenderWidth(const std::string & text, const bool utf8_encoded = false);
	int getHeight(void);
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
	int getSize(){return font.width;}
#else
	int getSize(){return font.font.pix_width;}
#endif
	int setSize(int isize);

	Font(FBFontRenderClass *render, FTC_FaceID faceid, const int isize, const fontmodifier _stylemodifier);
	~Font(){}
};

class FBFontRenderClass
{
		struct fontListEntry
		{
			char *filename, *style, *family;
			fontListEntry *next;
			~fontListEntry();
		}
		*font;

		FT_Library	library;
		FTC_Manager	cacheManager;	/* the cache manager               */
		FTC_ImageCache	imageCache;	/* the glyph image cache           */
		FTC_SBitCache	sbitsCache;	/* the glyph small bitmaps cache   */

		FTC_FaceID getFaceID(const char * const family, const char * const style);
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
		FT_Error getGlyphBitmap(FTC_ImageType font, FT_ULong glyph_index, FTC_SBit *sbit);
#else
		FT_Error getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit);
#endif

	public:
		pthread_mutex_t     render_mutex;

		FT_Error FTC_Face_Requester(FTC_FaceID face_id, FT_Face* aface);


		static FT_Error myFTC_Face_Requester(FTC_FaceID  face_id,
		                                     FT_Library  library,
		                                     FT_Pointer  request_data,
		                                     FT_Face*    aface);

		//FT_Face getFace(const char *family, const char *style);
		Font *getFont(const char * const family, const char * const style, int size);

    std::string getFamily(const char * const filename) const;

		const char * AddFont(const char * const filename, const bool make_italics = false);

		FBFontRenderClass();
		~FBFontRenderClass();

		friend class Font;
};

#endif
