#include <lib/gdi/font.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

// use this for init Freetype...
#include <ft2build.h>
#include FT_FREETYPE_H

#include <lib/base/eerror.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/grc.h>
#include <lib/system/elock.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

//#define HAVE_FRIBIDI
// until we have it in the cdk

#ifdef HAVE_FRIBIDI
#include <fribidi/fribidi.h>
#endif

#include <map>
        
fontRenderClass *fontRenderClass::instance;

static pthread_mutex_t ftlock = 
	PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;   // RECURSIVE

#ifdef FT_NEW_CACHE_API
static FTC_ImageType cache_current_font=0;
#else
static FTC_Font cache_current_font=0;
#endif

struct fntColorCacheKey
{
	gRGB start, end;
	fntColorCacheKey(const gRGB &start, const gRGB &end)
		: start(start), end(end)
	{
	}
	bool operator <(const fntColorCacheKey &c) const
	{
		if (start < c.start)
			return 1;
		else if (start == c.start)
			return end < c.end;
		return 0;
	}
};

std::map<fntColorCacheKey,gLookup> colorcache;

static gLookup &getColor(const gPalette &pal, const gRGB &start, const gRGB &end)
{
	fntColorCacheKey key(start, end);
	std::map<fntColorCacheKey,gLookup>::iterator i=colorcache.find(key);
	if (i != colorcache.end())
		return i->second;
	gLookup &n=colorcache.insert(std::pair<fntColorCacheKey,gLookup>(key,gLookup())).first->second;
	eDebug("[FONT] creating new font color cache entry %02x%02x%02x%02x .. %02x%02x%02x%02x", start.a, start.r, start.g, start.b,
		end.a, end.r, end.g, end.b);
	n.build(16, pal, start, end);
/*	for (int i=0; i<16; i++)
		eDebugNoNewLine("%02x|%02x%02x%02x%02x ", (int)n.lookup[i], pal.data[n.lookup[i]].a, pal.data[n.lookup[i]].r, pal.data[n.lookup[i]].g, pal.data[n.lookup[i]].b);
	eDebug("");*/
	return n;
}

fontRenderClass *fontRenderClass::getInstance()
{
	return instance;
}

FT_Error myFTC_Face_Requester(FTC_FaceID	face_id,
															FT_Library	library,
															FT_Pointer	request_data,
															FT_Face*		aface)
{
	return ((fontRenderClass*)request_data)->FTC_Face_Requester(face_id, aface);
}


FT_Error fontRenderClass::FTC_Face_Requester(FTC_FaceID	face_id, FT_Face* aface)
{
	fontListEntry *font=(fontListEntry *)face_id;

	if (!font)
		return -1;

//	eDebug("[FONT] FTC_Face_Requester (%s)", font->face.c_str());
	int error;
	if ((error=FT_New_Face(library, font->filename.c_str(), 0, aface)))
	{
		eDebug(" failed: %s", strerror(error));
		return error;
	}
	FT_Select_Charmap(*aface, ft_encoding_unicode);
	return 0;
}

FTC_FaceID fontRenderClass::getFaceID(const eString &face)
{
	for (fontListEntry *f=font; f; f=f->next)
	{
		if (f->face == face)
			return (FTC_FaceID)f;
	}
	return 0;
}

#ifdef FT_NEW_CACHE_API
FT_Error fontRenderClass::getGlyphBitmap(FTC_ImageType font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	FT_Error res=FTC_SBitCache_Lookup(sbitsCache, font, glyph_index, sbit, NULL);
#else
FT_Error fontRenderClass::getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	FT_Error res=FTC_SBit_Cache_Lookup(sbitsCache, font, glyph_index, sbit);
#endif
	return res;
}

eString fontRenderClass::AddFont(const eString &filename, const eString &name, int scale)
{
	eDebugNoNewLine("[FONT] adding font %s...", filename.c_str());
	int error;
	fontListEntry *n=new fontListEntry;

	n->scale=scale;
	FT_Face face;
	singleLock s(ftlock);

	if ((error=FT_New_Face(library, filename.c_str(), 0, &face)))
		eFatal(" failed: %s", strerror(error));

	n->filename=filename;
	n->face=name;
	FT_Done_Face(face);

	n->next=font;
	eDebug("OK (%s)", n->face.c_str());
	font=n;

	return n->face;
}

fontRenderClass::fontListEntry::~fontListEntry()
{
}

fontRenderClass::fontRenderClass()
	:fb(fbClass::getInstance())
{
	init_fontRenderClass();
}
void fontRenderClass::init_fontRenderClass()
{
	instance=this;
	eDebug("[FONT] initializing lib...");

	if (FT_Init_FreeType(&library))
	{
		eDebug("[FONT] initializing failed.");
		return;
	}

	eDebug("[FONT] loading fonts...");
	font=0;
	int maxbytes=4*1024*1024;
	eDebug("[FONT] Intializing font cache, using max. %dMB...", maxbytes/1024/1024);

	if (FTC_Manager_New(library, 8, 8, maxbytes, myFTC_Face_Requester, this, &cacheManager))
	{
		eDebug("[FONT] initializing font cache failed!");
		return;
	}

	if (!cacheManager)
	{
		eDebug("[FONT] initializing font cache manager error.");
		return;
	}

	if (FTC_SBitCache_New(cacheManager, &sbitsCache))
	{
		eDebug("[FONT] initializing font cache sbit failed!");
		return;
	}

	if (FTC_ImageCache_New(cacheManager, &imageCache))
		eDebug("[FONT] initializing font cache imagecache failed!");
}

float fontRenderClass::getLineHeight(const gFont& font)
{
	if (!instance)
		return 0;
	Font *fnt = getFont( font.family.c_str(), font.pointSize);
	if (!fnt)
		return 0;
	singleLock s(ftlock);
#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;
	scaler.face_id = fnt->font.face_id;
	scaler.width   = fnt->font.width;
	scaler.height  = fnt->font.height;
	scaler.pixel   = true;
	if (FTC_Manager_LookupSize(cacheManager, &scaler, &fnt->size)<0)
#else
	FT_Face current_face;
	if (FTC_Manager_Lookup_Size(cacheManager, &fnt->font.font, &current_face, &fnt->size)<0)
#endif
	{
		delete fnt;
		eDebug("FTC_Manager_Lookup_Size failed!");
		return 0;
	}
#ifdef FT_NEW_CACHE_API
	int linegap=fnt->size->metrics.height-(fnt->size->metrics.ascender+fnt->size->metrics.descender);
	float height=(fnt->size->metrics.ascender+fnt->size->metrics.descender+linegap/2.0)/64;
#else
	int linegap=current_face->size->metrics.height-(current_face->size->metrics.ascender+current_face->size->metrics.descender);
	float height=(current_face->size->metrics.ascender+current_face->size->metrics.descender+linegap/2.0)/64;
#endif
	delete fnt;
	return height;
}


fontRenderClass::~fontRenderClass()
{
	singleLock s(ftlock);
	while(font)
	{
		fontListEntry *f=font;
		font=font->next;
		delete f;
	}
//	auskommentiert weil freetype und enigma die kritische masse des suckens ueberschreiten. 
//	FTC_Manager_Done(cacheManager);
//	FT_Done_FreeType(library);
}

Font *fontRenderClass::getFont(const eString &face, int size, int tabwidth)
{
	FTC_FaceID id=getFaceID(face);
	if (!id)
		return 0;
	return new Font(this, id, size * ((fontListEntry*)id)->scale / 100, tabwidth);
}

Font::Font(fontRenderClass *render, FTC_FaceID faceid, int isize, int tw)
	:renderer(render), ref(0), tabwidth(tw),  height(isize)
{
#ifdef FT_NEW_CACHE_API
	font.face_id = faceid;
	font.width = isize;
	font.height = isize;
	font.flags = FT_LOAD_DEFAULT;
#else
	font.font.face_id=faceid;
	font.font.pix_width	= isize;
	font.font.pix_height = isize;
	font.image_type = ftc_image_grays;
#endif
	if (tabwidth==-1)
		tabwidth=8*isize;
//	font.image_type |= ftc_image_flag_autohinted;
}

FT_Error Font::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

Font::~Font()
{
}

void Font::lock()
{
	ref++;
}

void Font::unlock()
{
	ref--;
	if (!ref)
		delete this;
}

int eTextPara::appendGlyph(Font *current_font, FT_Face current_face, FT_UInt glyphIndex, int flags, int rflags)
{
	FTC_SBit glyph;
	if (current_font->getGlyphBitmap(glyphIndex, &glyph))
		return 1;

	int nx=cursor.x();

	nx+=glyph->xadvance;
	
	if (
			(rflags&RS_WRAP) && 
			(nx >= area.right())
		)
	{
		int cnt = 0;
		glyphString::reverse_iterator i(glyphs.rbegin());
		while (i != glyphs.rend())
		{
			if (i->flags&(GS_ISSPACE|GS_ISFIRST))
				break;
			cnt++;
			++i;
		} 
		if (i != glyphs.rend()
			&& ((i->flags&(GS_ISSPACE|GS_ISFIRST))==GS_ISSPACE)
			&& cnt )
		{
			--i;
			int linelength=cursor.x()-i->x;
			i->flags|=GS_ISFIRST;
			ePoint offset=ePoint(i->x, i->y);
			newLine(rflags);
			offset-=cursor;
			do
			{
				i->x-=offset.x();
				i->y-=offset.y();
				i->bbox.moveBy(-offset.x(), -offset.y());
			}
			while (i-- != glyphs.rbegin()); // rearrange them into the next line
			cursor+=ePoint(linelength, 0);  // put the cursor after that line
		}
		else
		{
			if (cnt)
			{
				newLine(rflags);
				flags|=GS_ISFIRST;
			}
		}
	}

	int xadvance=glyph->xadvance, kern=0;
	
	if (previous && use_kerning)
	{
		FT_Vector delta;
		FT_Get_Kerning(current_face, previous, glyphIndex, ft_kerning_default, &delta);
		kern=delta.x>>6;
	}

	pGlyph ng;
	ng.bbox.setLeft( (flags&GS_ISFIRST|cursor.x()-1)+glyph->left );
	ng.bbox.setTop( cursor.y() - glyph->top );
	ng.bbox.setWidth( glyph->width );
	ng.bbox.setHeight( glyph->height );

	xadvance+=kern;

	ng.x=cursor.x()+kern;
	ng.y=cursor.y();
	ng.w=xadvance;
	ng.font=current_font;
	ng.font->lock();
	ng.glyph_index=glyphIndex;
	ng.flags=flags;

	glyphs.push_back(ng);

	cursor+=ePoint(xadvance, 0);
	previous=glyphIndex;
	return 0;
}

void eTextPara::calc_bbox()
{
	boundBox.setLeft( 32000 );
	boundBox.setTop( 32000 );
	boundBox.setRight( -32000 );         // for each glyph image, compute its bounding box, translate it,
	boundBox.setBottom( -32000 );
	// and grow the string bbox

	for ( glyphString::iterator i(glyphs.begin()); i != glyphs.end(); ++i)
	{
		if ( i->flags & GS_ISSPACE )
			continue;
		if ( i->bbox.left() < boundBox.left() )
			boundBox.setLeft( i->bbox.left() );
		if ( i->bbox.top() < boundBox.top() )
			boundBox.setTop( i->bbox.top() );
		if ( i->bbox.right() > boundBox.right() )
			boundBox.setRight( i->bbox.right() );
		if ( i->bbox.bottom() > boundBox.bottom() )
			boundBox.setBottom( i->bbox.bottom() );
	}
//	eDebug("boundBox left = %i, top = %i, right = %i, bottom = %i", boundBox.left(), boundBox.top(), boundBox.right(), boundBox.bottom() );
	if ( glyphs.size() )
		bboxValid=1;
}

void eTextPara::newLine(int flags)
{
	if (maximum.width()<cursor.x())
		maximum.setWidth(cursor.x());
	cursor.setX(left);
	previous=0;
#ifdef FT_NEW_CACHE_API
	int linegap=current_font->size->metrics.height-(current_font->size->metrics.ascender+current_font->size->metrics.descender);
	cursor+=ePoint(0, (current_font->size->metrics.ascender+current_font->size->metrics.descender+linegap*1/2)>>6);
#else
	int linegap=current_face->size->metrics.height-(current_face->size->metrics.ascender+current_face->size->metrics.descender);
	cursor+=ePoint(0, (current_face->size->metrics.ascender+current_face->size->metrics.descender+linegap*1/2)>>6);
#endif
	if (maximum.height()<cursor.y())
		maximum.setHeight(cursor.y());
	previous=0;
}

eTextPara::~eTextPara()
{
	clear();
	if (refcnt>=0)
		eFatal("verdammt man der war noch gelockt :/\n");
}

void eTextPara::destroy()
{
	if (!refcnt--)
		delete this;
}

eTextPara *eTextPara::grab()
{
	refcnt++;
	return this;
}

void eTextPara::setFont(const gFont &font)
{
	if (refcnt)
		eFatal("mod. after lock");

	Font *fnt=fontRenderClass::getInstance()->getFont(font.family.c_str(), font.pointSize);
	if (!fnt)
		eWarning("FONT '%s' MISSING!", font.family.c_str());
	setFont(fnt,
		fontRenderClass::getInstance()->getFont(replacement_facename.c_str(), font.pointSize));
}

eString eTextPara::replacement_facename;

void eTextPara::setFont(Font *fnt, Font *replacement)
{
	if (refcnt)
		eFatal("mod. after lock");

	if (!fnt)
		return;

	if (current_font)
	{
		if ( !current_font->ref )
			delete current_font;
	}
	current_font=fnt;
	if (replacement_font)
	{
		if (!replacement_font->ref)
			delete replacement_font;
	}
	replacement_font=replacement;
	singleLock s(ftlock);

#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;
#endif

			// we ask for replacment_font first becauseof the cache
	if (replacement_font)
	{
#ifdef FT_NEW_CACHE_API
		scaler.face_id = replacement_font->font.face_id;
		scaler.width   = replacement_font->font.width;
		scaler.height  = replacement_font->font.height;
		scaler.pixel   = true;
		if (FTC_Manager_LookupSize(fontRenderClass::instance->cacheManager,
				&scaler, &replacement_font->size)<0)
#else
		if (FTC_Manager_Lookup_Size(fontRenderClass::instance->cacheManager, 
				&replacement_font->font.font, &replacement_face, 
				&replacement_font->size)<0)
#endif
		{
			eDebug("FTC_Manager_Lookup_Size failed!");
			return;
		}
	}
	if (current_font)
	{
#ifdef FT_NEW_CACHE_API
		scaler.face_id = current_font->font.face_id;
		scaler.width   = current_font->font.width;
		scaler.height  = current_font->font.height;
		scaler.pixel   = true;

		if (FTC_Manager_LookupSize(fontRenderClass::instance->cacheManager, &scaler, &current_font->size)<0)
#else
		if (FTC_Manager_Lookup_Size(fontRenderClass::instance->cacheManager, &current_font->font.font, &current_face, &current_font->size)<0)
#endif
		{
			eDebug("FTC_Manager_Lookup_Size failed!");
			return;
		}
	}
#ifdef FT_NEW_CACHE_API
	cache_current_font=&current_font->font;
	previous=0;
	use_kerning=FT_HAS_KERNING(current_font->size->face);
#else
	cache_current_font=&current_font->font.font;
	previous=0;
	use_kerning=FT_HAS_KERNING(current_face);
#endif
}

void
shape (std::vector<unsigned long> &string, const std::vector<unsigned long> &text);

int eTextPara::renderString(const eString &string, int rflags)
{
	singleLock s(ftlock);
	
	if (refcnt)
		eFatal("mod. after lock");

	if (!current_font)
		return -1;
		
	if (cursor.y()==-1)
	{
#ifdef FT_NEW_CACHE_API
		cursor=ePoint(area.x(), area.y()+(current_font->size->metrics.ascender>>6));
#else
		cursor=ePoint(area.x(), area.y()+(current_face->size->metrics.ascender>>6));
#endif
		left=cursor.x();
	}
		
#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;
	if (&current_font->font != cache_current_font)
	{
		scaler.face_id = current_font->font.face_id;
		scaler.width   = current_font->font.width;
		scaler.height  = current_font->font.height;
		scaler.pixel   = true;
		if (FTC_Manager_LookupSize(fontRenderClass::instance->cacheManager, &scaler, &current_font->size)<0)
#else
	if (&current_font->font.font != cache_current_font)
	{
		if (FTC_Manager_Lookup_Size(fontRenderClass::instance->cacheManager, &current_font->font.font, &current_face, &current_font->size)<0)
#endif
		{
			eDebug("FTC_Manager_Lookup_Size failed!");
			return -1;
		}
#ifdef FT_NEW_CACHE_API
		cache_current_font=&current_font->font;
#else
		cache_current_font=&current_font->font.font;
#endif
	}
	
	std::vector<unsigned long> uc_string, uc_visual;
	uc_string.reserve(string.length());
	
	eString::const_iterator p(string.begin());

	while(p != string.end())
	{
		unsigned int unicode=*p++;

		if (unicode & 0x80) // we have (hopefully) UTF8 here, and we assume that the encoding is VALID
		{
			if ((unicode & 0xE0)==0xC0) // two bytes
			{
				unicode&=0x1F;
				unicode<<=6;
				if (p != string.end())
					unicode|=(*p++)&0x3F;
			} else if ((unicode & 0xF0)==0xE0) // three bytes
			{
				unicode&=0x0F;
				unicode<<=6;
				if (p != string.end())
					unicode|=(*p++)&0x3F;
				unicode<<=6;
				if (p != string.end())
					unicode|=(*p++)&0x3F;
			} else if ((unicode & 0xF8)==0xF0) // four bytes
			{
				unicode&=0x07;
				unicode<<=6;
				if (p != string.end())
					unicode|=(*p++)&0x3F;
				unicode<<=6;
				if (p != string.end())
					unicode|=(*p++)&0x3F;
				unicode<<=6;
				if (p != string.end())
					unicode|=(*p++)&0x3F;
			}
		}
		uc_string.push_back(unicode);
	}

	std::vector<unsigned long> uc_shape;

		// character -> glyph conversion
	shape(uc_shape, uc_string);
	
		// now do the usual logical->visual reordering
#ifdef HAVE_FRIBIDI	
	FriBidiCharType dir=FRIBIDI_TYPE_ON;
	{
		int size=uc_shape.size();
		uc_visual.resize(size);
		// gaaanz lahm, aber anders geht das leider nicht, sorry.
		FriBidiChar array[size], target[size];
		std::copy(uc_shape.begin(), uc_shape.end(), array);
		fribidi_log2vis(array, size, &dir, target, 0, 0, 0);
		uc_visual.assign(target, target+size);
	}
#else
	uc_visual=uc_shape;
#endif

	glyphs.reserve(uc_visual.size());

	int nextflags = 0;
	
	for (std::vector<unsigned long>::const_iterator i(uc_visual.begin());
		i != uc_visual.end(); ++i)
	{
		int isprintable=1;
		int flags = nextflags;
		nextflags = 0;
		if (!(rflags&RS_DIRECT))
		{
			switch (*i)
			{
			case '\\':
			{
				unsigned long c = *(i+1);
				switch (c)
				{
					case 'n':
						i++;
						goto newline;
					case 't':
						i++;
						goto tab;
					case 'r':
						i++;
						goto nprint;
					default:
					;
				}
				break;
			}
			case '\t':
tab:		isprintable=0;
				cursor+=ePoint(current_font->tabwidth, 0);
				cursor-=ePoint(cursor.x()%current_font->tabwidth, 0);
				break;
			case 0x8A:
			case 0xE08A:
			case '\n':
newline:isprintable=0;
				newLine(rflags);
				nextflags|=GS_ISFIRST;
				break;
			case '\r':
			case 0x86: case 0xE086:
			case 0x87: case 0xE087:
nprint:	isprintable=0;
				break;
			case ' ':
				flags|=GS_ISSPACE;
			default:
				break;
			}
		}
		if (isprintable)
		{
			FT_UInt index;

#ifdef FT_NEW_CACHE_API
			index=(rflags&RS_DIRECT)? *i : FT_Get_Char_Index(current_font->size->face, *i);
#else
			index=(rflags&RS_DIRECT)? *i : FT_Get_Char_Index(current_face, *i);
#endif

			if (!index)
			{
#ifdef FT_NEW_CACHE_API
				if (replacement_font)
					index=(rflags&RS_DIRECT)? *i : FT_Get_Char_Index(replacement_font->size->face, *i);
#else
				if (replacement_face)
					index=(rflags&RS_DIRECT)? *i : FT_Get_Char_Index(replacement_face, *i);
#endif

				if (!index)
					eDebug("unicode %d ('%c') not present", *i, *i);
				else
#ifdef FT_NEW_CACHE_API
					appendGlyph(replacement_font, replacement_font->size->face, index, flags, rflags);
#else
					appendGlyph(replacement_font, replacement_face, index, flags, rflags);
#endif
			} else
#ifdef FT_NEW_CACHE_API
				appendGlyph(current_font, current_font->size->face, index, flags, rflags);
#else
				appendGlyph(current_font, current_face, index, flags, rflags);
#endif
		}
	}
	bboxValid=false;
	calc_bbox();
#ifdef HAVE_FRIBIDI
	if (dir & FRIBIDI_MASK_RTL)
		realign(dirRight);
#endif
	return 0;
}

void eTextPara::blit(gPixmapDC &dc, const ePoint &offset, const gRGB &background, const gRGB &foreground)
{
	singleLock s(ftlock);
	
	if (!current_font)
		return;

#ifdef FT_NEW_CACHE_API
	FTC_ScalerRec scaler;
	if (&current_font->font != cache_current_font)
	{
		scaler.face_id = current_font->font.face_id;
		scaler.width   = current_font->font.width;
		scaler.height  = current_font->font.height;
		scaler.pixel   = true;
		if (FTC_Manager_LookupSize(fontRenderClass::instance->cacheManager, &scaler, &current_font->size)<0)
#else
	if (&current_font->font.font != cache_current_font)
	{
		if (FTC_Manager_Lookup_Size(fontRenderClass::instance->cacheManager, &current_font->font.font, &current_face, &current_font->size)<0)
#endif
		{
			eDebug("FTC_Manager_Lookup_Size failed!");
			return;
		}
#ifdef FT_NEW_CACHE_API
		cache_current_font=&current_font->font;
#else
		cache_current_font=&current_font->font.font;
#endif
	}

	gPixmap &target=dc.getPixmap();

	register int opcode;
	gColor *lookup8=0;
	__u32 lookup32[16];
		
	if (target.bpp == 8)
	{
		if (target.clut.data)
		{
			lookup8=getColor(target.clut, background, foreground).lookup;
			opcode=0;
		} else
			opcode=1;
	} else if (target.bpp == 32)
	{
		opcode=3;
		if (target.clut.data)
		{
			lookup8=getColor(target.clut, background, foreground).lookup;
			for (int i=0; i<16; ++i)
				lookup32[i]=((target.clut.data[lookup8[i]].a<<24)|
					(target.clut.data[lookup8[i]].r<<16)|
					(target.clut.data[lookup8[i]].g<<8)|
					(target.clut.data[lookup8[i]].b))^0xFF000000;
		} else
		{
			for (int i=0; i<16; ++i)
				lookup32[i]=(0x010101*i)|0xFF000000;
		}
	} else
	{
		eWarning("can't render to %dbpp", target.bpp);
		return;
	}
	
	eRect clip(0, 0, target.x, target.y);
	clip&=dc.getClip();

	int buffer_stride=target.stride;

	for (glyphString::iterator i(glyphs.begin()); i != glyphs.end(); ++i)
	{
		static FTC_SBit glyph_bitmap;
		if (fontRenderClass::instance->getGlyphBitmap(&i->font->font, i->glyph_index, &glyph_bitmap))
			continue;
		int rx=i->x+glyph_bitmap->left + offset.x();
		int ry=i->y-glyph_bitmap->top  + offset.y();
		__u8 *d=(__u8*)(target.data)+buffer_stride*ry+rx*target.bypp;
		__u8 *s=glyph_bitmap->buffer;
		register int sx=glyph_bitmap->width;
		int sy=glyph_bitmap->height;
		if ((sy+ry) >= clip.bottom())
			sy=clip.bottom()-ry;
		if ((sx+rx) >= clip.right())
			sx=clip.right()-rx;
		if (rx < clip.left())
		{
			int diff=clip.left()-rx;
			s+=diff;
			sx-=diff;
			rx+=diff;
			d+=diff*target.bypp;
		}
		if (ry < clip.top())
		{
			int diff=clip.top()-ry;
			s+=diff*glyph_bitmap->pitch;
			sy-=diff;
			ry+=diff;
			d+=diff*buffer_stride;
		}
		if (sx>0)
			for (int ay=0; ay<sy; ay++)
			{
				if (!opcode)		// 4bit lookup to 8bit
				{
					register __u8 *td=d;
					register int ax;
					for (ax=0; ax<sx; ax++)
					{	
						register int b=(*s++)>>4;
						if(b)
							*td++=lookup8[b];
						else
							td++;
					}
				} else if (opcode == 1)	// 8bit direct
				{
					register __u8 *td=d;
					register int ax;
					for (ax=0; ax<sx; ax++)
					{	
						register int b=*s++;
						*td++^=b;
					}
				} else
				{
					register __u32 *td=(__u32*)d;
					register int ax;
					for (ax=0; ax<sx; ax++)
					{	
						register int b=(*s++)>>4;
						if(b)
							*td++=lookup32[b];
						else
							td++;
					}
				}
				s+=glyph_bitmap->pitch-sx;
				d+=buffer_stride;
			}
	}
}

void eTextPara::realign(int dir)	// der code hier ist ein wenig merkwuerdig.
{
	if (refcnt)
		eFatal("mod. after lock");

	if (dir==dirLeft)
		return;

	glyphString::iterator begin(glyphs.begin()), c(glyphs.begin()), end(glyphs.begin()), last;
	while (c != glyphs.end())
	{
		int linelength=0;
		int numspaces=0, num=0;
		begin=end;
		
		ASSERT( end != glyphs.end());
		
			// zeilenende suchen
		do {
			last=end;
			++end;
		} while ((end != glyphs.end()) && (!(end->flags&GS_ISFIRST)));
			// end zeigt jetzt auf begin der naechsten zeile
		
		for (c=begin; c!=end; ++c)
		{
				// space am zeilenende skippen
			if ((c==last) && (c->flags&GS_ISSPACE))
				continue;

			if (c->flags&GS_ISSPACE)
				numspaces++;
			linelength+=c->w;
			num++;
		}

		if (!num)		// line mit nur einem space
			continue;

		switch (dir)
		{
		case dirRight:
		case dirCenter:
		{
			int offset=area.width()-linelength;
			if (dir==dirCenter)
				offset/=2;
			offset+=area.left();
			while (begin != end)
			{
				begin->bbox.moveBy(offset-begin->x,0);
				begin->x=offset;
				offset+=begin->w;
				++begin;
			}
			break;
		}
		case dirBlock:
		{
			if (end == glyphs.end())		// letzte zeile linksbuendig lassen
				continue;
			int spacemode;
			if (numspaces)
				spacemode=1;
			else
				spacemode=0;
			if ((!spacemode) && (num<2))
				break;
			int off=(area.width()-linelength)*256/(spacemode?numspaces:(num-1));
			int curoff=0;
			while (begin != end)
			{
				int doadd=0;
				if ((!spacemode) || (begin->flags&GS_ISSPACE))
					doadd=1;
				begin->x+=curoff>>8;
				begin->bbox.moveBy(curoff>>8,0);
				if (doadd)
					curoff+=off;
				++begin;
			}
			break;
		}
		}
	}
	bboxValid=false;
	calc_bbox();
}

void eTextPara::clear()
{
	singleLock s(ftlock);

	if ( current_font && !current_font->ref )
		delete current_font;

	if ( replacement_font && !replacement_font->ref )
		delete replacement_font;

	for (glyphString::iterator i(glyphs.begin()); i!=glyphs.end(); ++i)
		i->font->unlock();
	glyphs.clear();
}

eAutoInitP0<fontRenderClass> init_fontRenderClass(eAutoInitNumbers::graphic-1, "Font Render Class");
