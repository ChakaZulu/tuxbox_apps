#ifndef __lcd_h
#define __lcd_h

#include <asm/types.h>
#include <core/base/esize.h>
#include <core/base/erect.h>

#define LCD_CONTRAST_MIN 0
#define LCD_CONTRAST_MAX 63
#define LCD_BRIGHTNESS_MIN 0
#define LCD_BRIGHTNESS_MAX 255

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
	static eDBoxLCD *instance;
public:
	static eDBoxLCD *getInstance();
	int switchLCD(int state);
	int setLCDParameter(int brightness, int contrast);
	eDBoxLCD();
	~eDBoxLCD();
	void update();
};


#endif
