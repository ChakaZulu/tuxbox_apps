/***********************************************************************
 * ein weiteres wenig sinnvolles programm für die dbox2 (C) 2001 Ge0rG *
 *                                                                     *
 * Dieses Programm unterliegt der GPL, also nix klauen ;o)             *
 ***********************************************************************/

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "lcd-ks0713.h"
#include "../../bmp2raw/ani.h"

typedef unsigned char screen_t[LCD_BUFFER_SIZE];


int lcd_fd;

void clr() {
	if (ioctl(lcd_fd,LCD_IOCTL_CLEAR) < 0) {
		perror("clr()");
		exit(1);
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


int main(int argc, char *args[]) {
	FILE *f;
	struct ani_header ah;
	screen_t *ani;
	int t;

	f = fopen(args[1], "r");
	fread(&ah, 1, sizeof(ah), f);
	ani = malloc(sizeof(screen_t)*ah.count);
	fread(ani, sizeof(screen_t), ah.count, f);
	fclose(f);

	init();

	t = 0;
	while (1) {
		draw_screen(ani[t]);
		t++;
		if (t == ah.count) t = 0;
		usleep(ah.delay);
	}
	clr();
	return 0;
}
