#ifndef __lcd_h
#define __lcd_h

#include <asm/types.h>
#include <core/base/esize.h>
#include <core/base/erect.h>

class eLCD
{
protected:
	static eLCD *primary;
	eSize res;
	unsigned char *_buffer;
	int lcdfd;
	int _stride;
	int locked;
public:
	int lock();
	void unlock();

	eLCD(eSize size);
	virtual ~eLCD();
	static eLCD *getPrimary();

	__u8 *buffer() { return (__u8*)_buffer; }
	int stride() { return _stride; }
	eSize size() { return res; }
	
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
