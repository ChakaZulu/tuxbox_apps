// misc font / text rendering functions

#include <stdio.h>
#include <stdlib.h>
#include "fontrenderer.h"

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


fontRenderClass::fontRenderClass(CFrameBuffer *fb)
{
	framebuffer = fb;
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
	return new Font(framebuffer, this, id, size);
}

Font::Font(CFrameBuffer *fb, fontRenderClass *render, FTC_FaceID faceid, int isize)
{
	framebuffer=fb;
	renderer=render;
	font.font.face_id=faceid;
	font.font.pix_width  = isize;
	font.font.pix_height = isize;
	font.image_type = ftc_image_grays;
	font.image_type |= ftc_image_flag_autohinted;
}

FT_Error Font::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

int Font::getHeight(void)
{
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return 1;
	}
	return ((size->metrics.height+32) >> 6); // was (why?): (size->metrics.height >> 6)*3/4 + 4;
}

void Font::RenderString(int x, int y, int width, const char *string, unsigned char color, int boxheight)
{
	if (FTC_Manager_Lookup_Size(renderer->cacheManager, &font.font, &face, &size)<0)
	{ 
		printf("FTC_Manager_Lookup_Size failed!\n");
		return;
	}
  
	int left=x;
	int step_y=((size->metrics.height+32) >> 6); // was (why?): (size->metrics.height >> 6 )*3/4 + 4;

	// ----------------------------------- box upper end (this is NOT a font metric, this is our method for y centering)
	//
	// *  -------------------------------- y=baseline+ascender+linegap/2
	// |
	// |  --------------------*----------- y=baseline+ascender
	// |                     * *
	// h                    *   *
	// e     *        *    *     *
	// i      *      *     *******
	// g       *    *      *     *
	// h        *  *       *     *
	// t  -------**--------*-----*-------- y=baseline
	// |         *
	// |        *
	// |  -----**------------------------- y=baseline+descender   // descender is a NEGATIVE value
	// |
	// *  -------------------------------- y=baseline+descender-linegap/2 == YCALLER
	//
	// ----------------------------------- box lower end (this is NOT a font metric, this is our method for y centering)
	
	// height = ascender + -1*descender + linegap           // descender is negative!
	
	// now we adjust the given y value (which is the line marked as YCALLER) to be the baseline after adjustment:
	int linegap = size->metrics.height - size->metrics.ascender + size->metrics.descender;
	y += ((size->metrics.descender - (linegap>>1)) >> 6);
	// y coordinate now gives font baseline which is used for drawing

	//printf("a %lx - d %lx + l %lx = h %lx\n",
	//    (size->metrics.ascender) >> 6,
	//    (size->metrics.descender) >> 6,
	//    (size->metrics.height-size->metrics.ascender+size->metrics.descender) >> 6,
	//    (size->metrics.height) >> 6
	//);

	// caution: this only works if we print a single line of text
	// if we have multiple lines, don't use boxheight or specify boxheight==0.
	// if boxheight is !=0, we further adjust y, so that text is y-centered in the box
	if(boxheight){
		if(boxheight>step_y)			// this is a real box (bigger than text)
			y -= ((boxheight-step_y)>>1);
		else if(boxheight<0)			// this normally would be wrong, so we just use it to define a "border"
			y += (boxheight>>1);		// half of border value at lower end, half at upper end
	}
		
	int lastindex=-1;
	FT_Vector kerning;
	int pen1=-1; // "pen" positions for kerning, pen2 is "x"
  
	for (; *string; string++)
	{
		FTC_SBit glyph;
		//if ((x + size->metrics.x_ppem > (left+width)) || (*string=='\n'))
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
		if(x+glyph->xadvance >= left+width)
			return;

		//kerning
		if(lastindex>=0){ // are we writing the 2nd or later character ?
			FT_Get_Kerning(face,lastindex,index,0,&kerning);
			int k=(kerning.x+32)>>6; // kerning!
			// printf("Kerning (%c,%c) == %d\n", *(string-1),*string,k);
			x+=k;
		}

		int rx=x+glyph->left;
		int ry=y-glyph->top;
    
		__u8 *d=framebuffer->lfb + framebuffer->Stride()*ry + rx;
		__u8 *s=glyph->buffer;
    
		for (int ay=0; ay<glyph->height; ay++)
		{
			__u8 *td=d;
			int ax=0;
			int w=glyph->width;
  
			for (; ax<w; ax++)
			{
				int c = (*s++>>5);
				*td++=color + c;
			}
			s+=glyph->pitch-ax;
			d+=framebuffer->Stride();
		}
		x+=glyph->xadvance+1;
		if(pen1>x)
			x=pen1;
		pen1=x;
		lastindex=index;
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
        int lastindex=-1;
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
		if(lastindex>=0){ // are we writing the 2nd or later character ?
			FT_Get_Kerning(face,lastindex,index,0,&kerning);
			int k=(kerning.x+32)>>6; // kerning!
			// printf("Kerning (%c,%c) == %d\n", *(string-1),*string,k);
			x+=k;
		}
    
		x+=glyph->xadvance+1;
		if(pen1>x)
		        x=pen1;
		pen1=x;
		lastindex=index;
	}
	return x;
}

