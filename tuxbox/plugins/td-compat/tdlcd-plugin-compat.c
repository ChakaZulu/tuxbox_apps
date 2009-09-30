/* normally, those are in the dbox driver headers */
#define LCD_ROWS	8
#define LCD_COLS	120
#define LCD_BUFFER_SIZE	(8 * 120)
#define LCD_PIXEL_OFF	0
#define LCD_PIXEL_ON	1
#define LCD_PIXEL_INV	2

/* ioctl compat */
#define LCD_IOCTL_CLEAR	IOC_LCD_CLEAR

/* this function converts raw dbox2 lcd data to raw TD LCD data.
   might be handy for other plugins, too */
void dbox2_to_tdLCD(const int __lcd_fd, const unsigned char *__dbox_lcd_buf)
{
#define __TD_LCD_X	128
#define __LCD_Y		64
#define __TD_LCD_SIZE (__TD_LCD_X * __LCD_Y / 8)
/* the offset is needed to center the 120 pix image on the 128 pix display
   the "+1" comes from the TD display being shifted one pixel to the left */
#define __OFF ((__TD_LCD_X - LCD_COLS) / 2 + 1)
	int __x, __y, __i;
	unsigned char __tdlcd[__TD_LCD_SIZE];
	unsigned char __tbit = 0x80 >> __OFF;
	unsigned char __sbit;
	memset(&__tdlcd, 0, __TD_LCD_SIZE);
	for (__x = __OFF; __x < __OFF + LCD_COLS; __x++)
	{
		for (__y = 0; __y < __LCD_Y / 8; __y++)
		{
			__sbit = 0x01;
			for (__i = 0; __i < 8; __i++)
			{
				if (__dbox_lcd_buf[__x - __OFF + __y * LCD_COLS] & __sbit)
					__tdlcd[__x / 8 + (__y * 8 + __i) * __TD_LCD_X / 8] |= __tbit;
				__sbit <<= 1;
			}
		}
		__tbit >>= 1;
		if (!__tbit)
			__tbit = 0x80;
	}
	if (write(__lcd_fd, &__tdlcd, __TD_LCD_SIZE) < 0)
		perror("dbox2_to_tdLCD write");
}
