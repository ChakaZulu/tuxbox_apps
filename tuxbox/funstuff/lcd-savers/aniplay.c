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
#include "ani.h"

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
	int t, loop, loopcount;

	if (argc < 2) {
		printf("aniplay (C) 2001 Ge0rG\n");
		printf("	%s <file.ani> [<loopcount>]\n");
		exit(1);
	}
	loopcount=0;
	if (argc==3) loopcount=atoi(args[2]);

	f = fopen(args[1], "r");
	fread(&ah, 1, sizeof(ah), f);
	ani = malloc(sizeof(screen_t)*ah.count);
	fread(ani, sizeof(screen_t), ah.count, f);
	fclose(f);


	init();

	t = 0;
	loop = 0;
	while ((loop<loopcount) || (loopcount==0)) {
		for (t=0; t<ah.count; t++) {
			draw_screen(ani[t]);
			usleep(ah.delay);
		}
		loop++;
	}
	clr();
	return 0;
}
