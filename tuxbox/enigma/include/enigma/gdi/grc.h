#ifndef __grc_h
#define __grc_h

/*
	gPainter ist die high-level version. die highlevel daten werden zu low level opcodes ueber
	die gRC-queue geschickt und landen beim gDC der hardwarespezifisch ist, meist aber auf einen
	gPixmap aufsetzt (und damit unbeschleunigt ist).
*/

#include <pthread.h>
#include <deque>

#include <core/base/estring.h>
#include <core/base/erect.h>
#include <core/system/elock.h>
#include <core/gdi/gpixmap.h>


class eTextPara;

class gDC;
struct gOpcode
{
	enum Opcode
	{
		begin,
		
		renderText,
		renderPara,
		
		fill,
		blit,

		setPalette,
		mergePalette,
		
		line,
		
		clip,
		
		flush,
		end,
		
		shutdown
	} opcode;

	union
	{
		struct
		{
			eRect *area;
		} begin;
		
		struct
		{
			eRect *area;
			gColor *color;
		} fill;

		struct
		{
			gFont *font;
			eRect *area;
			eString *text;
		} renderText;

		struct
		{
			ePoint *offset;
			eTextPara *textpara;
		} renderPara;

		struct
		{
			gPalette *palette;
		} setPalette;
		
		struct
		{
			gPixmap *pixmap;
			ePoint *position;
			eRect *clip;
		} blit;

		struct
		{
			gPixmap *target;
		} mergePalette;
		
		struct
		{
			ePoint *start, *end;
			gColor *color;
		} line;

		struct
		{
			eRect *clip;
		} clip;
	} parm;

	int flags;
	
	gDC *dc;
};

class gRC
{
	static gRC *instance;
	
	static void *thread_wrapper(void *ptr);
	pthread_t the_thread;
	void *thread();
	
	eLock queuelock;
	std::deque<gOpcode> queue;
	
public:
	gRC();
	virtual ~gRC();

	void submit(const gOpcode &opcode);
	static gRC &getInstance();
};

class gPainter
{
	gDC &dc;
	gRC &rc;
	friend class gRC;

	gOpcode *beginptr;

			/* paint states */	
	eRect cliparea;
	gFont font;
	gColor foregroundColor, backgroundColor;
	ePoint logicalZero;
	void begin(const eRect &rect);
	void end();
public:
	gPainter(gDC &dc, eRect rect=eRect());
	virtual ~gPainter();

	void setBackgroundColor(const gColor &color);
	void setForegroundColor(const gColor &color);

	void setFont(const gFont &font);
	void renderText(const eRect &position, const std::string &string, int flags=0);
	void renderPara(eTextPara &para);

	void fill(const eRect &area);
	
	void clear();
	
	void blit(gPixmap &src, ePoint pos, eRect clip=eRect(), int flags=0);

	void setPalette(gRGB *colors, int start=0, int len=256);
	void mergePalette(gPixmap &target);
	
	void line(ePoint start, ePoint end);

	void setLogicalZero(ePoint abs);
	void moveLogicalZero(ePoint rel);
	void resetLogicalZero();
	
	void clip(eRect clip);

	void flush();
};

class gDC
{
protected:
	eLock dclock;
public:
	virtual void exec(gOpcode *opcode)=0;
	virtual gPixmap &getPixmap()=0;
	virtual eSize getSize()=0;
	virtual const eRect &getClip()=0;
	virtual ~gDC();
	void lock() { dclock.lock(1); }
	void unlock() { dclock.unlock(1); }
};

class gPixmapDC: public gDC
{
protected:
	gPixmap *pixmap;
	eRect clip;

	void exec(gOpcode *opcode);
	gPixmapDC();
public:
	gPixmapDC(gPixmap *pixmap);
	virtual ~gPixmapDC();
	gPixmap &getPixmap() { return *pixmap; }
	const eRect &getClip() { return clip; }
	virtual eSize getSize() { return eSize(pixmap->x, pixmap->y); }
};

#endif
