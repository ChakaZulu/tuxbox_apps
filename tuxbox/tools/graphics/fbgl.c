/*
 * Framebuffer startup code for TinyGL
 *
 * This code also gets the keycodes from the input
 * event device (up, down, left, right and everything
 * else is treated as escape).
 *
 * The code is not guaranteed to work on all FBs
 * but it does work for an eNX-dbox2 and my PC.
 *
 * Take it as a proof of concept and to show
 * how the FB is dealt with. Read, learn and
 * make it better. ;)
 *
 * Requires the kernel 2.6 fb driver for the dbox2
 * (since it needs the RGB565 mode to work)
 *
 * One can use 720x288 as well, but 720x576 does not
 * fit into Demux-RAM as configured by default (double 
 * buffering is not possible in this case, single buffer 
 * would work, but does of course flicker).
 *
 * Yes, I know, it makes no sense really, I'm crazy, 
 * but I'm an OpenGL(R)-addict, so I *wanted* it to work
 * on the dbox2 :)
 *
 * My big thanks to Fabrice Bellard for his awesome
 * TinyGL.
 * 
 * Copyright (c) 2006 Carsten Juttner
 * All rights reserved.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

/* 
 * Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 * 	  this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * 	  this list of conditions and the following disclaimer in the documentation
 * 	  and/or other materials provided with the distribution.
 *  * Neither the name of the author nor the names of the contributors
 * 	  may be used to endorse or promote products derived from this software
 * 	  without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "GL/gl.h"
#include "GL/oscontext.h"

#define NOKEYDEFS
#include "ui.h"
#undef NOKEYDEFS

enum ui_keys {
	UI_UP = 0xe000,
	UI_DOWN,
	UI_LEFT,
	UI_RIGHT, 
	UI_ESCAPE
};

#define DEVFB "/dev/fb0"
#define DEVTTY "/dev/tty0"
#define DEVEVENT "/dev/input/event0"
static int devfb,devtty,devevent;
static int oldmode;
static char *fbmem;
static void *buffers[2];
static struct fb_var_screeninfo oldinfo;
static struct fb_var_screeninfo info;
static struct fb_fix_screeninfo fix;
static struct fb_cmap cmap;
static int dbl_buf = 0; /* double buffering */
ostgl_context *ctx;

static void setup_cmap( int bits )
{
	unsigned int i;
	cmap.start=0;
	cmap.len = 256;
	cmap.red = alloca( cmap.len * sizeof(uint16_t) );
	cmap.green = alloca( cmap.len * sizeof(uint16_t) );
	cmap.blue = alloca( cmap.len * sizeof(uint16_t) );
	cmap.transp = alloca( cmap.len * sizeof(uint16_t) );

	memset( cmap.red, 0, cmap.len * sizeof(uint16_t) );
	memset( cmap.green, 0, cmap.len * sizeof(uint16_t) );
	memset( cmap.blue, 0, cmap.len * sizeof(uint16_t) );
	memset( cmap.transp, 0, cmap.len * sizeof(uint16_t) );

	switch(bits) {
	case 16:
		cmap.len=64;
		// 5: 0000 0111 1122 2223
		// 6: 0000 0011 1111 2222
		for (i=0;i<cmap.len;i++) {
			cmap.green[i]= i << 10 | i << 4 | i >> 2         ; /* 6 */
			if (i>0x1f)
				continue;
			cmap.red[i]=   i << 11 | i << 6 | i << 1 | i >> 4; /* 5 */
			cmap.blue[i]=  i << 11 | i << 6 | i << 1 | i >> 4; /* 5 */
		}
		break;

	case 32:
		for (i=0;i<cmap.len;i++) {
			cmap.red[i]=   i<<8 | i;
			cmap.green[i]= i<<8 | i;
			cmap.blue[i]=  i<<8 | i;
		}
		break;
	default:
		fprintf(stderr,"Unknown cmap-type\n");
		return;
		break;

	}

	if (ioctl(devfb, FBIOPUTCMAP, &cmap )<0) {
		perror("FBIOGETCMAP");
	}
}

static int screeninfo_setup(const struct fb_var_screeninfo *in, const struct fb_var_screeninfo *out)
{
	if (ioctl(devfb, FBIOPUT_VSCREENINFO, in)<0) {
		perror("FBIOPUT_VSCREENINFO");
		return 1;
	}

	if (ioctl(devfb, FBIOGET_VSCREENINFO, out)<0) {
		perror("FBIOGET_VSCREENINFO");
		return 1;
	}
	return 0;
}

static int get_fixinfo( struct fb_fix_screeninfo *in )
{
	if (ioctl(devfb, FBIOGET_FSCREENINFO, in)<0) {
		perror("FBIOGET_FSCREENINFO");
		return 1;
	}
	return 0;
}

static void clear( unsigned char r, unsigned char g, unsigned char b )
{
	int w,h;
	uint16_t *ptr = (uint16_t*)fbmem;
	r >>= 3; g >>= 2; b >>= 3;
	r &= 0x1f; g &= 0x3f; b &= 0x1f;
	for (h=0;h<info.yres;h++) {
		for (w=0;w<info.xres;w++) {
			unsigned char hi,lo;
			/* rrrr rggg gggb bbbb */
			hi = (uint16_t)r << 3 | g >> 3;
			lo = (uint16_t)g << 5 | b;

			*ptr++ = (uint16_t)hi << 8 | lo;
		}
	}
}

static void FBSwap(void);
static int FBOpen(void)
{
	devtty = open(DEVTTY, O_RDWR);
	if (devtty<0) {
		perror("Unable to open " DEVTTY);
		return 1;
	}
	if (ioctl(devtty, KDGETMODE, &oldmode)<0) {
		perror("KDGETMODE");
		return 1;
	}
	
	if (ioctl(devtty, KDSETMODE, KD_GRAPHICS)<0) {
		perror("KDSETMODE");
		return 1;
	}

	devfb = open(DEVFB, O_RDWR);
	if (devfb<0) {
		perror("Unable to open " DEVFB);
		return 1;
	}

	devevent = open(DEVEVENT, O_RDONLY | O_NONBLOCK);
	if (devevent<0) {
		perror("Unable to open event device, no keyboard support");
	}

	if (ioctl(devfb, FBIOGET_VSCREENINFO, &oldinfo)<0) {
		perror("FBIOGET_VSCREENINFO");
		return 1;
	}

	memcpy(&info, &oldinfo, sizeof(struct fb_var_screeninfo));

	get_fixinfo(&fix);

	/* try to set up 16 bit RGB565 */
	info.xres = 360;
	info.xres_virtual = 360;
	info.yres = 288;
	info.yres_virtual = 288;

	info.bits_per_pixel = 16;
	info.red.offset = 11;
	info.red.length = 5;
	info.green.offset = 5;
	info.green.length = 6;
	info.blue.offset = 0;
	info.blue.length = 5;
	info.transp.offset = 0;
	info.transp.length = 0;

	if (screeninfo_setup(&info, &info)<0) {
		perror("setup of mode\n");
		return 1;
	}

	printf("resolution: %d x %d\nRGB%d%d%d\n",info.xres,info.yres,
							info.red.length,info.green.length,info.blue.length);
							
	if (info.bits_per_pixel!=16 || 
		info.red.length != 5 || 
		info.blue.length != 5 || 
		info.green.length != 6) {
		printf("Unable to set desired mode (RGB565)\n");
		return 1;
	}
	
	/* see if we can enable double buffering 
	   for the current resolution */
	info.yres_virtual = 2 * info.yres;
	screeninfo_setup(&info, &info);
	
	if (info.yres_virtual >= 2*info.yres) {
		printf("enabling double buffering (yres_virtual: %d)\n",info.yres_virtual);
		dbl_buf = 1;
	}

	get_fixinfo(&fix);

	fbmem = mmap(NULL, ((info.xres*info.yres_virtual*info.bits_per_pixel)>>3), 
							PROT_READ|PROT_WRITE, MAP_SHARED, devfb, 0);
	if (fbmem == (char *)-1) {
		perror("mmap");
		return 1;
	}

	if (fix.visual == FB_VISUAL_DIRECTCOLOR) {
		printf("setting up colour maps\n");
		setup_cmap(16);
	}

	/* connecting it all together */
	buffers[0] = fbmem;
	if (dbl_buf) {
		buffers[1] = fbmem + ((info.xres*info.yres*info.bits_per_pixel)>>3);
		ctx = ostgl_create_context(info.xres, info.yres, 16, buffers, 2);
		FBSwap(); /* calls make_current */
	} else {
		ctx = ostgl_create_context(info.xres, info.yres, 16, buffers, 1);
		ostgl_make_current(ctx, 0);
	}

	clear(0,0,0);
	return 0;
}

static void FBClose(void)
{
	ostgl_delete_context(ctx);
	munmap(fbmem, (info.xres*info.yres_virtual*info.bits_per_pixel)>>3);

	if (ioctl(devfb, FBIOPUT_VSCREENINFO, &oldinfo)<0) {
		perror("FBIOPUT_VSCREENINFO");
	}

	if (ioctl(devtty, KDSETMODE, KD_TEXT)<0) {
		perror("KDSETMODE");
	}
	if (devevent>0)
		close(devevent);

	if (devfb>0)
		close(devfb);

	if (devtty>0)
		close(devtty);
}

static int FBGetMsg(void)
{
	struct input_event ev;
	int ui_key;
	int res;
	
	if (devevent<0)
		return 0;
	
	res = read(devevent,&ev,sizeof (struct input_event));
	if (res<0 && errno!=EAGAIN) {
		perror("read input_event");
	}

	if (ev.type!=EV_KEY)
		return 0;

	switch (ev.code) {
	case KEY_UP:
		ui_key = UI_UP;
		break;
	case KEY_DOWN:
		ui_key = UI_DOWN;
		break;
	case KEY_LEFT:
		ui_key = UI_LEFT;
		break;
	case KEY_RIGHT:
		ui_key = UI_RIGHT;
		break;
	case KEY_ESC:
	default:
		ui_key = UI_ESCAPE;
		break;
	}
	key(ui_key,0);
	return 0;
}

static void FBSwap(void)
{
	static int toggle = 0;
	if ( !dbl_buf )
		return;
		
	info.yoffset = toggle?info.yres:0;
	if (ioctl(devfb, FBIOPAN_DISPLAY, &info)<0) {
		perror("FBIOPAN_DISPLAY");
	}
	toggle^=1;
	ostgl_make_current(ctx,toggle);
}

int ui_loop(int argc, char **argv, const char *name)
{
	if (FBOpen())
		return 1;
		
	init();
	reshape(info.xres, info.yres);
	while (!FBGetMsg()) {
		idle();
	}
	FBClose();
	return 0;
}

void tkSwapBuffers(void)
{
	FBSwap();
}
