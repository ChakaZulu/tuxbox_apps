/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * svga.c - the svgalib interface
 * Basically hacked from x.c by {ganesh,sitaram}@cse.iitb.ernet.in
 */

#include <vncviewer.h>

#include <fcntl.h>
#include <unistd.h>

#include "fbgl.h"

#define INVALID_PIXEL 0xffffffff
#define COLORMAP_SIZE 256

unsigned long BGR233ToPixel[COLORMAP_SIZE];

#if 0
static void CopyBGR233ToScreen(CARD8 *buf, int x, int y, int width,int height);
#endif

/*
 * CopyDataToScreen.
 */

void
CopyDataToScreen(CARD8 *buf, int x, int y, int width, int height)
{
	gl_putbox(x, y, width, height, buf);
}

#if 0
/*
 * CopyBGR233ToScreen.
 */

static void
CopyBGR233ToScreen(CARD8 *buf, int x, int y, int width, int height)
{
    int p, q;
    void *screenData = global_framebuffer.v_buf;
    CARD8 *scr8 = ((CARD8 *)screenData) + y * si.framebufferWidth + x;
    CARD16 *scr16 = ((CARD16 *)screenData) + y * si.framebufferWidth + x;
    CARD32 *scr32 = ((CARD32 *)screenData) + y * si.framebufferWidth + x;

    switch (myFormat.bitsPerPixel) {

    case 8:
	for (q = 0; q < height; q++) {
	    for (p = 0; p < width; p++) {
		*(scr8++) = BGR233ToPixel[*(buf++)];
	    }
	    scr8 += si.framebufferWidth - width;
	}
	break;

    case 16:
	for (q = 0; q < height; q++) {
	    for (p = 0; p < width; p++) {
		*(scr16++) = BGR233ToPixel[*(buf++)];
	    }
	    scr16 += si.framebufferWidth - width;
	}
	break;

    case 32:
	for (q = 0; q < height; q++) {
	    for (p = 0; p < width; p++) {
		*(scr32++) = BGR233ToPixel[*(buf++)];
	    }
	    scr32 += si.framebufferWidth - width;
	}
	break;
    }
}
#endif

void
gl_setpalettecolor(int i, int r, int g, int b) {
	cleanup_and_exit("palette not implemented", EXIT_ERROR);
}

void
gl_copybox(int xsrc, int ysrc, int w, int h, int x, int y) {
	int j;
	IMPORT_FRAMEBUFFER_VARS

	if (y < ysrc) {
		Pixel *src, *dst;
		src = v_buf + ysrc * v_xsize + xsrc;
		dst = v_buf + y * v_xsize + x;

		for (j=0; j<h; j++) {
			memcpy(dst, src, w*sizeof(Pixel));
			src += v_xsize;
			dst += v_xsize;
		}
	} else if (y > ysrc) {
		Pixel *src, *dst;
		src = v_buf + (ysrc+h-1) * v_xsize + xsrc;
		dst = v_buf + (y+h-1) * v_xsize + x;
		
		for (j=h; j; j--) {
			memcpy(dst, src, w*sizeof(Pixel));
			src -= v_xsize;
			dst -= v_xsize;
		}
	} else /* y == ysrc */ {
		Pixel *src, *dst;
		src = v_buf + ysrc * v_xsize + xsrc;
		dst = v_buf + y * v_xsize + x;

		for (j=0; j<h; j++) {
			memmove(dst, src, w*sizeof(Pixel));
			src += v_xsize;
			dst += v_xsize;
		}
	}
	redraw_virt(x, y, w, h);
}

void
gl_fillbox(int x, int y, int w, int h, int col) {
	int i, j;
	IMPORT_FRAMEBUFFER_VARS

	for (j=0; j<h; j++) {
		Pixel *buf = v_buf + (y+j) * v_xsize + x;
		
		for (i=0; i<w; i++) {
//			*(buf++) = col;
			*(buf++) = 0x8000 | ((col & 0xF8) << 8) | ((col & 3) << 8) | ((col & 0xE000) >> 8) | ((col & 0x1F00) >> 8);
		}
	}
	redraw_virt(x, y, w, h);
//	printf("Fillbox %d\n",col);
}

void
gl_putbox(int x, int y, int w, int h, CARD8 *buf) {
	int j,k;
	Pixel *src, *dst;
	IMPORT_FRAMEBUFFER_VARS

	src = (Pixel*)buf;
	dst = v_buf + y * v_xsize + x;
	
	for (j=0; j<h; j++) {
//		memcpy(dst, src, w * sizeof(Pixel));
		for(k=0; k < w ; k++)
		{
			dst[k] = 0x8000 | ((src[k] & 0xF8) << 8) | ((src[k] & 3) << 8) | ((src[k] & 0xE000) >> 8) | ((src[k] & 0x1F00) >> 8);
		}
		src += w;
		dst += v_xsize;
	}
	redraw_virt(x, y, w, h);
//	printf("Putbox \n");
}

