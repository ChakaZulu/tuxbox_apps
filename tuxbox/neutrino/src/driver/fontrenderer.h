#ifndef __FONTRENDERER__
#define __FONTRENDERER__

#include "framebuffer.h"
#include <freetype/freetype.h>
#include <freetype/ftcache.h>
#include <freetype/cache/ftcchunk.h>
#include <freetype/cache/ftcglyph.h>
#include <freetype/cache/ftcimage.h>
#include <freetype/cache/ftcmanag.h>
#include <freetype/cache/ftcsbits.h>
#include <freetype/cache/ftlru.h>


class Font;
class fontRenderClass
{ 
	CFrameBuffer *framebuffer;

	struct fontListEntry
	{
		char *filename, *style, *family;
		fontListEntry *next;
		~fontListEntry();
	} *font;

	FT_Library		library;
	FTC_Manager		cacheManager;        /* the cache manager               */
	FTC_Image_Cache	imageCache;          /* the glyph image cache           */
	FTC_SBit_Cache	sbitsCache;          /* the glyph small bitmaps cache   */

	int AddFont(const char *filename);
	FTC_FaceID getFaceID(const char *family, const char *style);
	FT_Error getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit);

	public:
		FT_Error FTC_Face_Requester(FTC_FaceID  face_id,
                              FT_Face*    aface);


		static FT_Error myFTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Library  library,
                            FT_Pointer  request_data,
                            FT_Face*    aface);

		//FT_Face getFace(const char *family, const char *style);
		Font *getFont(const char *family, const char *style, int size);
		fontRenderClass(CFrameBuffer *fb);
		~fontRenderClass();


	friend class Font;
};

class Font
{
	CFrameBuffer	*framebuffer;
	FTC_Image_Desc	font;
	fontRenderClass *renderer;
	FT_Face			face;
	FT_Size			size;

	FT_Error getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit);
	
	public:
		void RenderString(int x, int y, int width, const char *string, unsigned char color);
		
		int getRenderWidth(const char *string);

		Font(CFrameBuffer *fb, fontRenderClass *render, FTC_FaceID faceid, int isize);
		~Font(){}
};

class FontsDef
{
        public:
                Font 
					*menu, 
					*menu_number,
					*menu_title, 
					*epg_title, 
					*epg_info1, 
					*epg_info2,
					*epg_date,
					*alert, 
					*channellist,

					*infobar_number,
					*infobar_channame,
					*infobar_info;
};



#endif
