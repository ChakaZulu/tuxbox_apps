#ifndef __pictureviewer_fbdisplay__
#define __pictureviewer_fbdisplay__
void * convertRGB2FB(unsigned char *rgbbuff, unsigned long count, int bpp, int *cpp);
extern void fb_display(unsigned char *rgbbuff, int x_size, int y_size, int x_pan, int y_pan, int x_offs, int y_offs);
extern void getCurrentRes(int *x, int *y);
#endif

