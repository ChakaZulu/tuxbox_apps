#ifndef __lcd_h
#define __lcd_h

#include <asm/types.h>
#include <qsize.h>
#include <qrect.h>

class eLCD
{
protected:
	static eLCD *primary;
	QSize res;
	unsigned char *_buffer;
	int lcdfd;
	int _stride;
	int locked;
public:
	int lock();
	void unlock();

	eLCD(QSize size);
	virtual ~eLCD();
	static eLCD *getPrimary();

	__u8 *buffer() { return (__u8*)_buffer; }
	int stride() { return _stride; }
	QSize size() { return res; }
	
	virtual void update()=0;
};

class eDBoxLCD: public eLCD
{
public:
	eDBoxLCD();
	~eDBoxLCD();
	void update();
};


#endif
