#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include "font.h"
#include "lcd.h"
#include "grc.h"
#include "elock.h"

#include <sys/types.h>
#include <unistd.h>


#include <freetype/freetype.h>

	/* the following header shouldn't be used in normal programs */
#include <freetype/internal/ftdebug.h>

	/* showing driver name */
#include <freetype/ftmodule.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftdriver.h>

#include "config.h"

fontRenderClass *fontRenderClass::instance;
static eLock ftlock;

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
	printf("[FONT] FTC_Face_Requester (%s)\n", font->face);

	int error;
	if ((error=FT_New_Face(library, font->filename, 0, aface)))
	{
		printf(" failed: %s\n", strerror(error));
		return error;
	}
	return 0;
}																																																																

FTC_FaceID fontRenderClass::getFaceID(const char *face)
{
	for (fontListEntry *f=font; f; f=f->next)
	{
		if (!strcmp(f->face, face))
			return (FTC_FaceID)f;
	}
	return 0;
}

FT_Error fontRenderClass::getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	FT_Error res=FTC_SBit_Cache_Lookup(sbitsCache, font, glyph_index, sbit);
	return res;
}

int fontRenderClass::AddFont(const char *filename)
{
	printf("[FONT] adding font %s...", filename);
	fflush(stdout);
	int error;
	fontListEntry *n=new fontListEntry;

	FT_Face face;
	eLocker lock(ftlock);
	if ((error=FT_New_Face(library, filename, 0, &face)))
	{
		printf(" failed: %s\n", strerror(error));
		return error;
	}
	strcpy(n->filename=new char[strlen(filename)+1], filename);
	strcpy(n->face=new char[strlen(face->family_name)+strlen(face->style_name)+2], face->family_name);
	if (face->style_name[0]!=' ')
		strcat(n->face, " ");
	strcat(n->face, face->style_name);
	FT_Done_Face(face);

	n->next=font;
	printf("OK (%s)\n", n->face);
	font=n;
	return 0;
}

fontRenderClass::fontListEntry::~fontListEntry()
{
	delete[] filename;
	delete[] face;
}

fontRenderClass::fontRenderClass(): fb(fbClass::getInstance())
{
	instance=this;
	printf("[FONT] initializing core...");
	{
		if (FT_Init_FreeType(&library))
		{
			printf("failed.\n");
			return;
		}
	}
	printf("\n[FONT] loading fonts...\n");
	fflush(stdout);
	font=0;
	AddFont(FONTDIR "/unmrs.pfa");
	AddFont(FONTDIR "/Marlett.ttf");
	AddFont(FONTDIR "/Courier_New_Bold.ttf");
	
	int maxbytes=4*1024*1024;
	printf("[FONT] Intializing font cache, using max. %dMB...", maxbytes/1024/1024);
	fflush(stdout);
	{
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
	}
	printf("\n");
	return;
}

fontRenderClass::~fontRenderClass()
{
	eLocker lock(ftlock);
	FTC_Manager_Done(cacheManager);
	FT_Done_FreeType(library);
}

Font *fontRenderClass::getFont(const char *face, int size, int tabwidth)
{
	FTC_FaceID id=getFaceID(face);
	if (!id)
		qFatal("face %s does not exist!", face);
	if (!id)
		return 0;
	return new Font(this, id, size, tabwidth);
}

Font::Font(fontRenderClass *render, FTC_FaceID faceid, int isize, int tw): tabwidth(tw)
{
	renderer=render;
	font.font.face_id=faceid;
	font.font.pix_width	= isize;
	font.font.pix_height = isize;
	font.image_type = ftc_image_grays;
	height=isize;
	if (tabwidth==-1)
		tabwidth=8*isize;
	ref=0;
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

static const int num_glyph=2048;
static pGlyph glyphs[num_glyph];
static int cptr=0;

pGlyph *allocateGlyph()
{
	int s=cptr;
	do
	{
		if (!(glyphs[cptr].flags&GS_USED))
		{
			glyphs[cptr].flags=GS_USED;
			return glyphs+cptr;
		}
		cptr++;
		if (cptr==num_glyph)
			cptr=0;
	} while (s!=cptr);
	pGlyph *g=new pGlyph;
	g->flags=GS_USED|GS_HEAP;
	return g;
}

int eTextPara::appendGlyph(FT_UInt glyphIndex, int flags)
{
	FTC_SBit glyph;
	if (current_font->getGlyphBitmap(glyphIndex, &glyph))
	{
		return 1;
	}
	if ((flags&GS_MYWRAP) && (cursor.x()+ glyph->xadvance) >= area.right())
	{
		glyphs.last();
		int cnt = 0;
		while (glyphs.current())
		{
			if (glyphs.current()->flags&(GS_ISSPACE|GS_ISFIRST))
				break;
			cnt++;
			glyphs.prev();
		}
		if (glyphs.current() && ((glyphs.current()->flags&(GS_ISSPACE|GS_ISFIRST))==GS_ISSPACE) && glyphs.next())		// skip space
		{
			int linelength=cursor.x()-glyphs.current()->x;
			glyphs.current()->flags|=GS_ISFIRST;
			QPoint offset=QPoint(glyphs.current()->x, glyphs.current()->y);
			newLine();
			offset-=cursor;
			while (glyphs.current())		// rearrange them into the next line
			{
				glyphs.current()->x-=offset.x();
				glyphs.current()->y-=offset.y();
				glyphs.next();
			}
			cursor+=QPoint(linelength, 0);	// put the cursor after that line
		} else
		{
	    if (cnt)
			{
				newLine();
				flags|=GS_ISFIRST;
			}
		}
		glyphs.last();
	}
	int xadvance=glyph->xadvance, kern=0;
	if (previous && use_kerning)
	{
		FT_Vector delta;
		FT_Get_Kerning(current_face, previous, glyphIndex, ft_kerning_default, &delta);
		kern=delta.x>>6;
	}
	pGlyph *ng=allocateGlyph();
	if (!ng)
		qFatal("too many glyphs active at once - increase limit in font.cpp!");
	ng->x=cursor.x()+kern;
	xadvance+=kern;
	ng->y=cursor.y();
	ng->w=xadvance;
	ng->font=current_font;
	ng->font->lock();
	ng->glyph_index=glyphIndex;
	ng->flags|=flags;
	glyphs.append(ng);
	cursor+=QPoint(xadvance, 0);
	previous=glyphIndex;
	return 0;
}

void eTextPara::newLine()
{
	if (maximum.width()<cursor.x())
		maximum.setWidth(cursor.x());
	cursor.setX(left);
	int linegap=current_face->size->metrics.height-(current_face->size->metrics.ascender+current_face->size->metrics.descender);
	cursor+=QPoint(0, (current_face->size->metrics.ascender+current_face->size->metrics.descender+linegap*1/2)>>6);
	if (maximum.height()<cursor.y())
		maximum.setHeight(cursor.y());
	previous=0;
}

static eLock refcntlck;

eTextPara::~eTextPara()
{
	clear();
	if (refcnt>=0)
		qFatal("verdammt man der war noch gelockt :/"); 
}

void eTextPara::destroy()
{
	eLocker lock(refcntlck);
	if (!refcnt--)
		delete this;
}

eTextPara *eTextPara::grab()
{
	eLocker lock(refcntlck);
	refcnt++;
	return this;
}

void eTextPara::setFont(const gFont &font)
{
	if (refcnt)
		qFatal("mod. after lock");
	setFont(fontRenderClass::getInstance()->getFont(font.family, font.pointSize));
}

void eTextPara::setFont(Font *fnt)
{
	if (refcnt)
		qFatal("mod. after lock");
	if (!fnt)
		return;
	if (current_font && !current_font->ref)
		delete current_font;
	current_font=fnt;
	eLocker lock(ftlock);
	if (FTC_Manager_Lookup_Size(fontRenderClass::instance->cacheManager, &current_font->font.font, &current_face, &current_font->size)<0)
	{
		printf("FTC_Manager_Lookup_Size failed!\n");
		return;
	}
	previous=0;
	if (cursor.y()==-1)
	{
		cursor=QPoint(area.x(), area.y()+(current_face->size->metrics.ascender>>6));
		left=cursor.x();
	}
	use_kerning=FT_HAS_KERNING(current_face);
}

int eTextPara::renderString(const QString &qstring, int rflags)
{
	eLocker lock(ftlock);

	if (refcnt)
		qFatal("mod. after lock");
	if (!current_font)
		return -1;
	const QChar *string=qstring.unicode();
	
	int len=qstring.length();
	while (len--)
	{
		int isprintable=1;
		int uc=string->unicode();
		int flags=0;
		if (rflags&RS_WRAP)
			 flags|=GS_MYWRAP;
		if (rflags&RS_DIRECT)
	  {
			isprintable=1;
		} else
		{
			switch (uc)
			{
			case '\t':
				isprintable=0;
				cursor+=QPoint(current_font->tabwidth, 0);
				cursor-=QPoint(cursor.x()%current_font->tabwidth, 0);
				break;
			case '\n':
				isprintable=0;
				newLine();
				flags|=GS_ISFIRST;
				break;
			case '\r':
				isprintable=0;
				break;
			case ' ':
				flags|=GS_ISSPACE;
			default:
				break;
			}
		}
		string++;
		if (isprintable)
		{
			FT_UInt index;
			index=(rflags&RS_DIRECT)?uc:FT_Get_Char_Index(current_face, uc);
			if (!index)
				; // qDebug("unicode %d ('%c') not present", uc, uc);
			else
				appendGlyph(index, flags);
		}
	}
	return 0;
}

void eTextPara::blit(gPixmapDC &dc, const QPoint &offset)
{
	eLocker lock(ftlock);

	gPixmap &target=dc.getPixmap();

	if (target.bpp != 8)
		qFatal("eTextPara::blit - can't render into %d bpp buffer", target.bpp);
		
	register int shift=target.clut?4:0;	// in grayscale modes use 8bit, else 4bit
	
	QRect clip(0, 0, target.x, target.y);
	clip&=dc.getClip();

	int buffer_stride=target.stride;

	for (QListIterator<pGlyph> i(glyphs); i.current(); ++i)
	{
		pGlyph *glyph=i.current();
		static FTC_SBit glyph_bitmap;
		if (fontRenderClass::instance->getGlyphBitmap(&glyph->font->font, glyph->glyph_index, &glyph_bitmap))
			continue;
		int rx=glyph->x+glyph_bitmap->left + offset.x();
		int ry=glyph->y-glyph_bitmap->top  + offset.y();
		__u8 *d=(__u8*)(target.data)+buffer_stride*ry+rx;
		__u8 *s=glyph_bitmap->buffer;
		register int sx=glyph_bitmap->width;
		int sy=glyph_bitmap->height;
		if ((sy+ry) > clip.bottom())
			sy=clip.bottom()-ry+1;
		if ((sx+rx) > clip.right())
			sx=clip.right()-rx+1;
		if (sx>0)
			for (int ay=0; ay<sy; ay++)
			{
				register __u8 *td=d;
				register int ax;
				for (ax=0; ax<sx; ax++)
				{	
					register int b=(*s++)>>shift;
					if(b)
						*td++|=b;
					else
						td++;
				}
				s+=glyph_bitmap->pitch-ax;
				d+=buffer_stride;
			}
	}
}

void eTextPara::realign(int dir)	// der code hier ist ein wenig merkwuerdig.
{
	QListIterator<pGlyph> begin(glyphs), c(begin), end(begin);
	pGlyph *last;
	if (dir==dirLeft)
		return;
	while (c.current())
	{
		int linelength=0;
		int numspaces=0, num=0;
		begin=end;
		
			// zeilenende suchen
		do {
			last=end.current();
			++end;
		} while (end.current() && (!(end.current()->flags&GS_ISFIRST)));
			// end zeigt jetzt auf begin der naechsten zeile
			
		for (c=begin; c!=end; ++c)
		{
				// space am zeilenende skippen
			if ((c.current()==last) && (c.current()->flags&GS_ISSPACE))
				continue;

			if (c.current()->flags&GS_ISSPACE)
				numspaces++;
			linelength+=c.current()->w;;
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
			while (begin != end)
			{
				begin.current()->x+=offset;
				++begin;
			}
			break;
		}
		case dirBlock:
		{
			if (!end.current())		// letzte zeile linksbuendig lassen
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
				if ((!spacemode) || (begin.current()->flags&GS_ISSPACE))
					doadd=1;
				begin.current()->x+=curoff>>8;
				if (doadd)
					curoff+=off;
				++begin;
			}
			break;
		}
		}
	}
}

void eTextPara::clear()
{
	eLocker lock(ftlock);
	for (glyphs.first(); glyphs.current(); glyphs.next())
	{
		glyphs.current()->font->unlock();
		if (glyphs.current()->flags&GS_HEAP)
			delete glyphs.current();
		else
			glyphs.current()->flags=0;
	}
	glyphs.clear();
}

QSize eTextPara::getExtend()
{
	QSize res=maximum;
			/* account last unfinished line */
	if (cursor.x() > res.width())
		res.setWidth(cursor.x());
	if (res.height()<cursor.y())
		res.setHeight(cursor.y()+1);
	return res;
}
