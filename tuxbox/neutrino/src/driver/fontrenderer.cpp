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
$Id: fontrenderer.cpp,v 1.16 2001/11/15 11:42:41 McClean Exp $

-- misc font / text rendering functions

$Log: fontrenderer.cpp,v $
Revision 1.16  2001/11/15 11:42:41  McClean
gpl-headers added

Revision 1.15  2001/10/18 10:55:56  field
Rendert jetzt auch Schriftfarben zwischen den Hauptfarben richtig (zb
COLOR_MENUCONTENT+1)

Revision 1.14  2001/10/16 19:11:16  rasc
-- CR LF --> LF in einigen Modulen

Revision 1.13  2001/10/14 15:48:16  McClean
use font-cache

Revision 1.12  2001/10/14 14:30:47  rasc
-- EventList Darstellung ueberarbeitet
-- kleiner Aenderungen und kleinere Bugfixes
-- locales erweitert..


*/


#include <stdio.h>
#include <stdlib.h>

#include "fontrenderer.h"
#include "../global.h"

// this method is recommended for FreeType >2.0.x:
#include <ft2build.h>
#include FT_FREETYPE_H


FT_Error fontRenderClass::myFTC_Face_Requester(FTC_FaceID  face_id,
                            FT_Library  library,
                            FT_Pointer  request_data,
                            FT_Face*    aface)
{
	return ((fontRenderClass*)request_data)->FTC_Face_Requester(face_id, aface);
}


fontRenderClass::fontRenderClass()
{
	printf("[FONT] initializing core...");
	if (FT_Init_FreeType(&library))
	{
		printf("failed.\n");
		return;
	}
	printf("\n[FONT] loading fonts...\n");
	fflush(stdout);
	font=0;

	AddFont("/usr/lib/fonts/Arial.ttf");
	AddFont("/usr/lib/fonts/Arial_Bold.ttf");
	AddFont("/usr/lib/fonts/Arial_Italic.ttf");
    AddFont("/usr/lib/fonts/Arial_Black.ttf");

	int maxbytes=4*1024*1024;
	printf("[FONT] Intializing font cache, using max. %dMB...", maxbytes/1024/1024);
	fflush(stdout);
	if (FTC_Manager_New(library, 0, 0, maxbytes, myFTC_Face_Requester, this, &cacheManager))
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

    pthread_mutex_init( &render_mutex, NULL );
  
	printf("\n");
}

fontRenderClass::~fontRenderClass()
{
	FTC_Manager_Done(cacheManager);
	FT_Done_FreeType(library);
}

FT_Error fontRenderClass::FTC_Face_Requester(FTC_FaceID face_id, FT_Face* aface)
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
	return new Font(this, id, size);
}

Font::Font(fontRenderClass *render, FTC_FaceID faceid, int isize)
{
	renderer=render;
	font.font.face_id=faceid;
	font.font.pix_width  = isize;
	font.font.pix_height = isize;
	font.image_type = ftc_image_grays;
	font.image_type |= ftc_image_flag_autohinted;

	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return;
	}
	// hack begin (this is a hack to get correct font metrics, didn't find any other way which gave correct values)
	FTC_SBit glyph;
	int index;

	index=FT_Get_Char_Index(face, 'M'); // "M" gives us ascender
	getGlyphBitmap(index, &glyph);
	int tM=glyph->top;

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
}

FT_Error Font::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

int Font::getHeight(void)
{
	return height;
}

void Font::RenderString(int x, int y, int width, const char *string, unsigned char color, int boxheight)
{
    pthread_mutex_lock( &renderer->render_mutex );

	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
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
	if(boxheight){
		if(boxheight>step_y)			// this is a real box (bigger than text)
			y -= ((boxheight-step_y)>>1);
		else if(boxheight<0)			// this normally would be wrong, so we just use it to define a "border"
			y += (boxheight>>1);		// half of border value at lower end, half at upper end
	}
		
	int lastindex=0; // 0 == missing glyph (never has kerning values)
	FT_Vector kerning;
	int pen1=-1; // "pen" positions for kerning, pen2 is "x"
  
	for (; *string; string++)
	{
		FTC_SBit glyph;
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
    
		// width clip
		if(x+glyph->xadvance > left+width)
        {
            pthread_mutex_unlock( &renderer->render_mutex );
			return;
        }

		//kerning
		if(use_kerning)
		{
		    FT_Get_Kerning(face,lastindex,index,0,&kerning);
		    x+=(kerning.x+32)>>6; // kerning!
		}

		int rx=x+glyph->left;
		int ry=y-glyph->top;
    
		__u8 *d=g_FrameBuffer->lfb + g_FrameBuffer->Stride()*ry + rx;
		__u8 *s=glyph->buffer;
    
        //color=((color+ 2)>>3)*8- 2;
        int coff=(color+ 2)%8;
		for (int ay=0; ay<glyph->height; ay++)
		{
			__u8 *td=d;
			int w=glyph->width;
 			int ax; 
			for (ax=0; ax<w; ax++)
			{
				int c = (*s++>>5)- coff; // c = 0..7
                if (c< 0)
                    c= 0;
				*td++=color + c;   // we use color as "base color" plus 7 consecutive colors for anti-aliasing
			}
			s+=glyph->pitch-ax;
			d+=g_FrameBuffer->Stride();
		}
		x+=glyph->xadvance+1;
		if(pen1>x)
			x=pen1;
		pen1=x;
		lastindex=index;
	}
    pthread_mutex_unlock( &renderer->render_mutex );
}

int Font::getRenderWidth(const char *string)
{
    pthread_mutex_lock( &renderer->render_mutex );

	int use_kerning=FT_HAS_KERNING(face);
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return -1;
	}

	int x=0;
        int lastindex=0; // 0==missing glyph (never has kerning)
        FT_Vector kerning;
        int pen1=-1; // "pen" positions for kerning, pen2 is "x"
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
                //kerning
		if(use_kerning){
			FT_Get_Kerning(face,lastindex,index,0,&kerning);
			x+=(kerning.x+32)>>6; // kerning!
		}
    
		x+=glyph->xadvance+1;
		if(pen1>x)
		        x=pen1;
		pen1=x;
		lastindex=index;
	}
    pthread_mutex_unlock( &renderer->render_mutex );

	return x;
}



