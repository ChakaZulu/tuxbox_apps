/*
$Id: fontrenderer.h,v 1.13 2001/10/14 14:30:47 rasc Exp $


$Log: fontrenderer.h,v $
Revision 1.13  2001/10/14 14:30:47  rasc
-- EventList Darstellung ueberarbeitet
-- kleiner Aenderungen und kleinere Bugfixes
-- locales erweitert..

Revision 1.12  2001/09/27 11:23:50  field
Numzap gefixt, kleiner Bugfixes

Revision 1.11  2001/09/26 16:24:17  rasc
- kleinere Aenderungen: Channel Num Zap fuer >999 Channels (Eutelsat/Astra) und eigener Font


*/

#ifndef __FONTRENDERER__
#define __FONTRENDERER__

#include "framebuffer.h"
#include "pthread.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

//#include <freetype/cache/ftcchunk.h>
//#include <freetype/cache/ftcglyph.h>

#include FT_CACHE_IMAGE_H
//#include <freetype/cache/ftcimage.h>

//#include <freetype/cache/ftcmanag.h>

#include FT_CACHE_SMALL_BITMAPS_H
//#include <freetype/cache/ftcsbits.h>


//#include <freetype/cache/ftlru.h>


class Font;
class fontRenderClass
{ 
	struct fontListEntry
	{
		char *filename, *style, *family;
		fontListEntry *next;
		~fontListEntry();
	} *font;

	FT_Library	library;
	FTC_Manager	cacheManager;        /* the cache manager               */
	FTC_Image_Cache	imageCache;          /* the glyph image cache           */
	FTC_SBit_Cache	sbitsCache;          /* the glyph small bitmaps cache   */

	int AddFont(const char *filename);
	FTC_FaceID getFaceID(const char *family, const char *style);
	FT_Error getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit);

	public:
        pthread_mutex_t     render_mutex;

		FT_Error FTC_Face_Requester(FTC_FaceID face_id, FT_Face* aface);


		static FT_Error myFTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Library  library,
                            FT_Pointer  request_data,
                            FT_Face*    aface);

		//FT_Face getFace(const char *family, const char *style);
		Font *getFont(const char *family, const char *style, int size);
		fontRenderClass();
		~fontRenderClass();


	friend class Font;
};

class Font
{
	FTC_Image_Desc	font;
	fontRenderClass *renderer;
	FT_Face		face;
	FT_Size		size;

	FT_Error getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit);

	// these are HACKED values, because the font metrics were unusable.
	int height,ascender,descender,upper,lower;
		
	public:
		void RenderString(int x, int y, int width, const char *string, unsigned char color, int boxheight=0);
		
		int getRenderWidth(const char *string);
		int getHeight(void);

		Font(fontRenderClass *render, FTC_FaceID faceid, int isize);
		~Font(){}
};

class FontsDef
{
        public:
                Font 
			*menu, 
			*menu_title,
			*menu_info,
			*epg_title, 
			*epg_info1, // epg_info1 should be same size as info2, but italic!
			*epg_info2, 
			*epg_date,

			*eventlist_title,
			*eventlist_itemLarge,
			*eventlist_datetime,
			*eventlist_itemSmall,

			*alert, 
			*channellist,
			*channellist_number,
			*channel_num_zap,

			*infobar_number,
			*infobar_channame,
			*infobar_info,
			*infobar_small,
			
			*fixedabr20; // fixed arial black 20
};

#endif



