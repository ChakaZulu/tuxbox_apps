#ifndef __grc_h
#define __grc_h

/*
	gPainter ist die high-level version. die highlevel daten werden zu low level opcodes ueber
	die gRC-queue geschickt und landen beim gDC der hardwarespezifisch ist, meist aber auf einen
	gPixmap aufsetzt (und damit unbeschleunigt ist).
*/

#include <pthread.h>
#include <qstring.h>
#include <qrect.h>
#include "elock.h"
#include "gpixmap.h"

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
			QRect *area;
		} begin;
		
		struct
		{
			QRect *area;
			gColor *color;
		} fill;

		struct
		{
			gFont *font;
			QRect *area;
			QString *text;
		} renderText;

		struct
		{
			QPoint *offset;
			eTextPara *textpara;
		} renderPara;

		struct
		{
			gPalette *palette;
		} setPalette;
		
		struct
		{
			gPixmap *pixmap;
			QPoint *position;
			QRect *clip;
		} blit;

		struct
		{
			gPixmap *target;
		} mergePalette;
		
		struct
		{
			QPoint *start, *end;
			gColor *color;
		} line;

		struct
		{
			QRect *clip;
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
	
	int fd[2];
	
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
	QRect cliparea;
	gFont font;
	gColor foregroundColor, backgroundColor;
	QPoint logicalZero;
	void begin(const QRect &rect);
	void end();
public:
	gPainter(gDC &dc, QRect rect=QRect());
	virtual ~gPainter();

	void setBackgroundColor(const gColor &color);
	void setForegroundColor(const gColor &color);

	void setFont(const gFont &font);
	void renderText(const QRect &position, const QString &string, int flags=0);
	void renderPara(eTextPara &para);

	void fill(const QRect &area);
	
	void clear();
	
	void blit(gPixmap &src, QPoint pos, QRect clip=QRect());

	void setPalette(gRGB *colors, int start=0, int len=256);
	void mergePalette(gPixmap &target);
	
	void line(QPoint start, QPoint end);

	void setLogicalZero(QPoint abs);
	void moveLogicalZero(QPoint rel);
	void resetLogicalZero();
	
	void clip(QRect clip);

	void flush();
};

class gDC
{
protected:
	eLock dclock;
public:
	virtual void exec(gOpcode *opcode)=0;
	virtual gPixmap &getPixmap()=0;
	virtual QSize getSize()=0;
	virtual const QRect &getClip()=0;
	virtual ~gDC();
	void lock() { dclock.lock(1); }
	void unlock() { dclock.unlock(1); }
};

class gPixmapDC: public gDC
{
protected:
	gPixmap *pixmap;
	QRect clip;

	void exec(gOpcode *opcode);
	gPixmapDC();
public:
	gPixmapDC(gPixmap *pixmap);
	virtual ~gPixmapDC();
	gPixmap &getPixmap() { return *pixmap; }
	const QRect &getClip() { return clip; }
	virtual QSize getSize() { return QSize(pixmap->x, pixmap->y); }
};

#endif
