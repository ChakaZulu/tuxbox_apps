/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

/*
$Id: fontrenderer.h,v 1.19 2002/02/10 14:17:34 McClean Exp $


$Log: fontrenderer.h,v $
Revision 1.19  2002/02/10 14:17:34  McClean
simplify usage (part 2)

Revision 1.18  2002/01/29 17:26:51  field
Jede Menge Updates :)

Revision 1.17  2002/01/03 20:03:20  McClean
cleanup

Revision 1.16  2001/12/05 21:38:09  rasc
gamelist: eigener Fontdef fuer 2-zeiliges Menue

Revision 1.15  2001/11/15 11:42:41  McClean
gpl-headers added

Revision 1.14  2001/10/16 19:11:16  rasc
-- CR LF --> LF in einigen Modulen

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
#include <string>

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
		}
		*font;

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
		void RenderString(int x, int y, int width, const char *text, unsigned char color, int boxheight=0);
		void RenderString(int x, int y, int width, string text, unsigned char color, int boxheight=0);

		int getRenderWidth(const char *text);
		int getRenderWidth(string text);
		int getHeight(void);

		Font(fontRenderClass *render, FTC_FaceID faceid, int isize);
		~Font()
		{}
}
;

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

		*gamelist_itemLarge,
		*gamelist_itemSmall,

		*alert,
		*channellist,
		*channellist_descr,
		*channellist_number,
		*channel_num_zap,

		*infobar_number,
		*infobar_channame,
		*infobar_info,
		*infobar_small;
}
;

#endif



