/* analclock (C) 2001 by Ge0rG

   Nein, nicht das was du jetzt denkst. Das hier ist eine Analoguhr
   und nix unanständiges :-P

   Ach ja, dieses Programm unterliegt der GPL, also nix klauen ;o)
*/

#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "lcd-ks0713.h"
#include "sinus.h"

#define SQRT 64*64+1


typedef unsigned char screen_t[LCD_BUFFER_SIZE];

int lcd_fd;

void clr() {
	if (ioctl(lcd_fd,LCD_IOCTL_CLEAR) < 0) {
		perror("clr()");
		exit(1);
	}
}

unsigned char Sqrt[SQRT];

void init_sqrt() {
	int pos, i, ii;
	i=0;
	ii=0;
	for (pos=0; pos<SQRT; pos++) {
		Sqrt[pos] = i;
		if (pos>ii) {
			i++;
			ii = i*i;
		}
	}
}


void init() {
	int i;
	if ((lcd_fd = open("/dev/dbox/lcd0",O_RDWR)) < 0) {
		perror("open(/dev/dbox/lcd0)");
		exit(1);
	}
	clr();

	i = LCD_MODE_BIN;
	if (ioctl(lcd_fd,LCD_IOCTL_ASC_MODE,&i) < 0) {
		perror("init(LCD_MODE_BIN)");
		exit(1);
	}
}

void draw_screen(screen_t s) {
	write(lcd_fd, s, LCD_BUFFER_SIZE);
}


void putpixel(int x, int y, char col, screen_t s) {
	int ofs = (y>>3)*LCD_COLS+x,
	    bit = y & 7;

	switch (col) {
	case LCD_PIXEL_ON:
		s[ofs] |= 1<<bit;
		break;
	case LCD_PIXEL_OFF:
		s[ofs] &= ~(1<<bit);
		break;
	case LCD_PIXEL_INV:
		s[ofs] ^= 1<<bit;
		break;
	}
}

void circle(int mx, int my, int r, char col, screen_t s) {
	int x, y;
	for (x=0; x<=r; x++) {
		y = Sqrt[r*r - x*x];
		putpixel(mx-x, my-y, col, s);
		putpixel(mx-y, my-x, col, s);
		putpixel(mx+x, my-y, col, s);
		putpixel(mx+y, my-x, col, s);
		putpixel(mx-x, my+y, col, s);
		putpixel(mx-y, my+x, col, s);
		putpixel(mx+x, my+y, col, s);
		putpixel(mx+y, my+x, col, s);
	}
}

#define MX 88
#define MY 32
#define RAD 27

void init_clock(screen_t s) {
	int m, x, y;
	memset(s, 0, LCD_BUFFER_SIZE);
	circle(MX, MY, 31, 1, s);
	putpixel(MX, MY, 1, s);

	for (m = 0; m < 60; m++) {
		x = MX - isin(-m*SIN_SIZE/30)*29/SIN_MUL;
		y = MY + isin(-SIN_SIZE/2 - m*SIN_SIZE/30)*29/SIN_MUL;
		putpixel(x, y, 1, s);
	}
}

void render_clock(screen_t back, screen_t s) {
        char timestr[50];
        struct timeb tb;
        struct tm *t;
	int bla, x, y, si, co;
        ftime(&tb);
	t = localtime(&tb.time);

	memcpy(s, back, LCD_BUFFER_SIZE);

	si = isin(-((t->tm_hour % 12)*60 + t->tm_min)*SIN_SIZE/360);
	co = isin(-SIN_SIZE/2 - ((t->tm_hour % 12)*60 + t->tm_min)*SIN_SIZE/360);
	for (bla=0; bla<20; bla++) {
		x = MX-si*bla/SIN_MUL;
		y = MY+co*bla/SIN_MUL;
		putpixel(x, y, 1, s);
		putpixel(x-1, y, 1, s);
		putpixel(x, y-1, 1, s);
		putpixel(x, y+1, 1, s);
		putpixel(x+1, y, 1, s);
	}

	si = isin(-(t->tm_min*60+t->tm_sec)*SIN_SIZE/1800);
	co = isin(-SIN_SIZE/2 - (t->tm_min*60+t->tm_sec)*SIN_SIZE/1800);
	for (bla=0; bla<30; bla++)
		putpixel(MX-si*bla/SIN_MUL, MY+co*bla/SIN_MUL, 1, s);

	x = MX - isin(-t->tm_sec*SIN_SIZE/30)*RAD/SIN_MUL;
	y = MY + isin(-SIN_SIZE/2 - t->tm_sec*SIN_SIZE/30)*RAD/SIN_MUL;
	putpixel(x, y, LCD_PIXEL_INV, s);
	
	printf("%i\n", t->tm_sec);
}

int main(int argc, char *args[]) {
	screen_t screen, back;

	init_sqrt();
	init();
	init_clock(back);
	while (1) {
		render_clock(back, screen);
		draw_screen(screen);
		sleep(1);
	}
	clr();
	return 0;
}
