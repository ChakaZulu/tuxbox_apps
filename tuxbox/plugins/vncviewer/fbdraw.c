#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "vncviewer.h"
#include "fbgl.h"
#include "overlay.h"

#if 0
#define DEBUG_CLIP
#endif

void
vp_pan_virt(int x0, int y0) {
	int w, h;
	IMPORT_FRAMEBUFFER_VARS
	
	if (p_landscape) {
		w = v_scale * pv_xsize;
		h = v_scale * pv_ysize;
	} else {
		w = v_scale * pv_ysize;
		h = v_scale * pv_xsize;
	}

	if (v_xsize > w) {
		/* virtual screen wider than display */
		if (x0 + w > v_xsize) x0 = v_xsize - w;
		if (x0 < 0) x0 = 0;
	} else {
		/* width fits inside display */
		if (-x0 + v_xsize > w) x0 = v_xsize - w;
		if (x0 > 0) x0 = 0;
	}

	if (v_ysize > h) {
		/* virtual screen higher than display */
		if (y0 + h > v_ysize) y0 = v_ysize - h;
		if (y0 < 0) y0 = 0;
	} else {
		/* height fits inside display */
		if (-y0 + v_ysize > h) y0 = v_ysize - h;
		if (y0 > 0) y0 = 0;
	}

	/* ensure that x0 and y0 are integer multiples of the scale */
	if (v_scale==1) {
		/* ensure that coordinates are even */
		x0 &= ~1;
		y0 &= ~1;
	} else {
		x0 = (x0 / v_scale) * v_scale;
		y0 = (y0 / v_scale) * v_scale;
	}

	global_framebuffer.v_x0 = x0;
	global_framebuffer.v_y0 = y0;

	if ( 0 ) {
		/* HACK to tell window manager about new geometry */
		FILE *o;

		o=fopen("/tmp/fbwm.geom", "w");
		fprintf(o, "%d %d %d %d\n", x0, y0,
			v_xsize <= w ? v_xsize : w,
			v_ysize <= h ? v_ysize : h);
		fclose(o);

		system("killall -USR2 xfwm");
	}
	
	redraw_phys_all();
}

void
grid_pan(int dx, int dy) {
	int x0, y0;
	int gx, gy;
	IMPORT_FRAMEBUFFER_VARS

	if (p_landscape) {
		gx = pv_xsize * v_scale / 2;
		gy = pv_ysize * v_scale / 2;
	} else {
#if 0
		int tmp = dy;
		dy = -dx;
		dx = tmp;
#endif

		gx = pv_ysize * v_scale / 2;
		gy = pv_xsize * v_scale / 2;
	}

	if ((v_x0/gx)*gx == v_x0) {
		/* on grid */
		x0 = v_x0 + gx*dx;
	} else {
		/* snap to grid */
		x0 = (v_x0/gx)*gx;
		if (dx>0) x0 += gx;
	}
	if ((v_y0/gy)*gy == v_y0) {
		/* on grid */
		y0 = v_y0 + gy*dy;
	} else {
		/* snap to grid */
		y0 = (v_y0/gy)*gy;
		if (dy>0) y0 += gy;
	}

	if (p_landscape) {
		if (dx>0 && v_x0 + v_scale * pv_xsize >= v_xsize) x0=v_x0;
		if (dy>0 && v_y0 + v_scale * pv_ysize >= v_ysize) y0=v_y0;
	} else {
		if (dx>0 && v_x0 + v_scale * pv_ysize >= v_xsize) x0=v_x0;
		if (dy>0 && v_y0 + v_scale * pv_xsize >= v_ysize) y0=v_y0;
	}
	if (dx==0) x0=v_x0;
	if (dy==0) y0=v_y0;

	if (x0 != v_x0 || y0 != v_y0)
		vp_pan_virt(x0, y0);
}

void
vp_pan(int pdx, int pdy) {
	int x0, y0;
	IMPORT_FRAMEBUFFER_VARS

	if (p_landscape) {
		x0 = v_x0 + pdx*v_scale;
		y0 = v_y0 + pdy*v_scale;
	} else {
		x0 = v_x0 + pdy*v_scale;
		y0 = v_y0 - pdx*v_scale;
	}

	vp_pan_virt(x0, y0);
}

void flip_orientation() {
	List *p;
	IMPORT_FRAMEBUFFER_VARS
	
	for (p=global_framebuffer.overlays->next; p; p = p->next) {
		fbvnc_overlay_t *ov = p->val;

		if (p_landscape) {
			global_framebuffer.p_landscape = 0;
			ov->x = ov->x0p;
			ov->y = ov->y0p;
		} else {
			global_framebuffer.p_landscape = 1;
			ov->x = ov->x0l;
			ov->y = ov->y0l;
		}
	}
	
	vp_pan(0, 0);
}

/* pixel format: 
 *   1111110000000000
 *   5432109876543210
 *   rrrrrggggggbbbbb
 *
 * intermediate format:
 *   33222222222211111111110000000000
 *   10987654321098765432109876543210
 *   xxRRRRRRRRRRGGGGGGGGGGBBBBBBBBBB
 *
 * maximum value = (max_pixel_component) * (scale*scale)
 *               = 63 * 16 = 63 * 4 * 4
 *               = 1008
 *		 < 1024 (10 bits per component)
 */

//#define RGBC(c) (((c) & 0x1f)<<1 | ((c)&0x7e0)<<5 | ((c)&0xf800)<<10)
#define RGBC(c) (((c) & 0x1f) | ((c)&0x3e0)<<5 | ((c)&0x7C00)<<10)
#define CR(c) ((c)>>20)
#define CG(c) ((c)>>10&1023)
#define CB(c) ((c)&1023)

typedef void aa_line_func(Pixel*src, Pixel*dst, int wp);

void
s1aa_line(Pixel *src, Pixel *dst, int wp)
{
	memcpy(dst, src, wp * sizeof(Pixel));
}

/*******************************************
 *        xp=0                  xp=p_xsize *
 *       +-----------------------------+   *
 *   yp=0|     xp             xp+wp    |   *
 *       |   yp+----------------+      |   *
 *       |     | j=0  p0P0  v v |      |   *
 *       |     | j=1  p1P1  v v |      |   *
 *       |     +----------------+      |   *
 *       |            v0v1             |   *
 *   yp= |            V0V1             |   *
 *p_ysize|                             |   *
 *       +-----------------------------+   *
 *                                         *
 *******************************************/
void
s1aa_line_portrait(Pixel *src, Pixel *dst, int hp)
{
	int i;
	IMPORT_FRAMEBUFFER_VARS

	for(i=0; i<hp; i+=2) {
		CARD32 v0, v1, p0, p1;
		v0 = *(CARD32*)src;
		v1 = *(CARD32*)(src-v_xsize);

		p0 = ((v1&0xffff) << 16) | (v0 & 0xffff);
		p1 =  (v1 & 0xffff0000)  | (v0 >> 16);

		*(CARD32*)dst           = p0;
		*(CARD32*)(dst+p_xsize) = p1;

		src += 2;
		dst += 2*p_xsize;
	}
}

Pixel
s2aa_pixel(Pixel *src)
{
	CARD32 rgb, p0, p1;
	IMPORT_FRAMEBUFFER_VARS

	/* Dirty trick: read two pixels at once. 
	 * The order of halfwords within a word doesn't actually
	 * matter, because they are averaged anyway.
	 */
	p0 = *(CARD32*)src;
	p1 = *(CARD32*)(src+v_xsize);

	rgb = RGBC(p0&0xffff) + RGBC(p0>>16)
	    + RGBC(p1&0xffff) + RGBC(p1>>16);

//	return (rgb>>3 & 0x1f) | (rgb>>7 & 0x7e0) | (rgb>>12 & 0xf800);
	return (rgb>>2 & 0x1f) | (rgb>>7 & 0x7e0) | (rgb>>12 & 0x7C00) | 0x8000;
}

void
s2aa_line(Pixel *src, Pixel *dst, int wp)
{
	int i;
	IMPORT_FRAMEBUFFER_VARS
	
	for (i=0; i<wp; i++) {
		*dst = s2aa_pixel(src);
		src += 2;
		dst++;
	}
}

void
s2aa_line_portrait(Pixel *src, Pixel *dst, int hp)
{
	int i;
	IMPORT_FRAMEBUFFER_VARS

	for(i=0; i<hp; i++) {
		*dst = s2aa_pixel(src);
		src += 2;
		dst += p_xsize;
	}
}

Pixel
s4aa_pixel(Pixel *src)
{
	CARD32 rgb = 0;
	int js;
	IMPORT_FRAMEBUFFER_VARS

	for (js=0; js<4; js++) {
		CARD32 p0, p1;
		Pixel *buf = src + js*v_xsize;

		p0 = *(CARD32*)(buf);
		p1 = *(CARD32*)(buf+2);

		rgb += RGBC(p0&0xffff) + RGBC(p0>>16)
		     + RGBC(p1&0xffff) + RGBC(p1>>16);
	}

//	return (rgb>>5 & 0x1f) | (rgb>>9 & 0x7e0) | (rgb>>14 & 0xf800);
	return (rgb>>4 & 0x1f) | (rgb>>9 & 0x7e0) | (rgb>>14 & 0x7c00) | 0x8000;

}

void
s4aa_line(Pixel *src, Pixel *dst, int wp)
{
	int i;
	IMPORT_FRAMEBUFFER_VARS
	
	for (i=0; i<wp; i++) {
		*dst = s4aa_pixel(src);
		src += 4;
		dst++;
	}
}

void
s4aa_line_portrait(Pixel *src, Pixel *dst, int hp)
{
	int i;
	IMPORT_FRAMEBUFFER_VARS

	for(i=0; i<hp; i++) {
		*dst = s4aa_pixel(src);
		src += 4;
		dst += p_xsize;
	}
}

void
s3aa_line(Pixel *src, Pixel *dst, int wp)
{
	int i;
	static bool initialized = 0;
	static Pixel pix_r[568], pix_g[568], pix_b[568];
	int xs;
	IMPORT_FRAMEBUFFER_VARS

	xs = v_xsize;
	if (!initialized) {
//		for (i=0; i<568; i++) {
		for (i=0; i<280; i++) {
			pix_b[i] = (i/9);
//			pix_g[i] = (i/9) <<5;
			pix_g[i] = (i/9) <<5;
//			pix_r[i] = (i/9) >>1<<11;
			pix_r[i] = (i/9) <<10;
		}
		initialized=1;
	}

	/***************************
	 *        src              *
	 *        [p0]p0 p0        *
	 *         p1 p1 p1        *
	 *         p2 p2 p2        *
	 *          r =====        *
	 *         === g ==        *
	 *         ====== b        *
	 *                         *
	 ***************************/

	for (i=0; i<wp; i++) {
		CARD32 p0, p1, p2, p;

		p0=RGBC(*(src     ))+RGBC(*(src+1     ))+RGBC(*(src+2     ));
		p1=RGBC(*(src + xs))+RGBC(*(src+1 + xs))+RGBC(*(src+2 + xs));
		p2=RGBC(*(src+2*xs))+RGBC(*(src+1+2*xs))+RGBC(*(src+2+2*xs));

		p = p0 + p1 + p2;
		*dst = pix_b[CB(p)] | pix_g[CG(p)] | pix_r[CR(p)] | 0x8000;

		src += 3;
		dst++;
	}
}

#if 0 /* obsolete */
void
s3aa_line_subpixel(Pixel *src, Pixel *dst, int wp)
{
	/* subpixel accuracy mode */

	/* FIXME: reads memory up to two pixels outside screen buffer -
	 * verify xv to avoid this
	 */

	CARD32 p0, p1, p2, p3;
	int i;
	static bool initialized = 0;
	static Pixel pix_r[1702], pix_g[1702], pix_b[1702];
	int xs;
	IMPORT_FRAMEBUFFER_VARS

	xs = v_xsize;
	if (!initialized) {
		for (i=0; i<1702; i++) {
			pix_b[i] = (i/27) >>1;
			pix_g[i] = (i/27) <<5;
			pix_r[i] = (i/27) >>1<<11;
		}
		initialized=1;
	}

	/***************************
	 *        src              *
	 *   p0 p1[p2]p3 p4 p5 p6  *
	 *   p0 p1 p2 p3 p4 p5 p6  *
	 *   p0 p1 p2 p3 p4 p5 p6  *
	 *   ------ r =====        *
	 *      ---=== g ==---     *
	 *         ====== b -----  *
	 *                         *
	 ***************************/

	p0=RGBC(*(src-2))+RGBC(*(src+xs-2))+RGBC(*(src+2*xs-2));
	p1=RGBC(*(src-1))+RGBC(*(src+xs-1))+RGBC(*(src+2*xs-1));
	p2=RGBC(*(src  ))+RGBC(*(src+xs  ))+RGBC(*(src+2*xs  ));
	p3=RGBC(*(src+1))+RGBC(*(src+xs+1))+RGBC(*(src+2*xs+1));

	for (i=0; i<wp; i++) {
		CARD32 p4, p5, p6;
		int r, g, b;

		p4=RGBC(*(src+2))+RGBC(*(src+xs+2))+RGBC(*(src+2*xs+2));
		p5=RGBC(*(src+3))+RGBC(*(src+xs+3))+RGBC(*(src+2*xs+3));
		p6=RGBC(*(src+4))+RGBC(*(src+xs+4))+RGBC(*(src+2*xs+4));
	
		r = CR(p0) + 2*CR(p1) + 3*CR(p2) + 2*CR(p3) + CR(p4);
		g = CG(p1) + 2*CG(p2) + 3*CG(p3) + 2*CG(p4) + CG(p5);
		b = CB(p2) + 2*CB(p3) + 3*CB(p4) + 2*CB(p5) + CB(p6);

		p0 = p3;
		p1 = p4;
		p2 = p5;
		p3 = p6;

		*dst = pix_b[b] | pix_g[g] | pix_r[r] | 0x8000;

		src += 3;
		dst++;
	}
}
#endif

void
s3aa_line_portrait(Pixel *src, Pixel *dst, int hp)
{
	int i;
	static bool initialized = 0;
	static Pixel pix_r[568], pix_g[568], pix_b[568];
	int xs;
	IMPORT_FRAMEBUFFER_VARS

	xs = v_xsize;
	if (!initialized) {
		for (i=0; i<568; i++) {
			pix_b[i] = (i/9) >>1;
			pix_g[i] = (i/9) <<5;
			pix_r[i] = (i/9) >>1<<11;
		}
		initialized=1;
	}

	/***************************
	 *              src        *
	 *         p2 p1[p0]       *
	 *         p2 p1 p0        *
	 *         p2 p1 p0        *
	 *          r =====        *
	 *         === g ==        *
	 *         ====== b        *
	 *                         *
	 ***************************/

	for (i=0; i<hp; i++) {
		CARD32 p0, p1, p2, p;

		p0=RGBC(*(src     ))+RGBC(*(src+1     ))+RGBC(*(src+2     ));
		p1=RGBC(*(src + xs))+RGBC(*(src+1 + xs))+RGBC(*(src+2 + xs));
		p2=RGBC(*(src+2*xs))+RGBC(*(src+1+2*xs))+RGBC(*(src+2+2*xs));

		p = p0 + p1 + p2;
		*dst = pix_b[CB(p)] | pix_g[CG(p)] | pix_r[CR(p)] | 0x8000;

		src += 3;
		dst += p_xsize;
	}
}

void
draw_border(int xp, int yp, int wp, int hp,
	int oxp, int oyp, int owp, int ohp)
{
	Pixel *buf;
	int j,k;
	IMPORT_FRAMEBUFFER_VARS

#ifdef DEBUG_CLIP
	fprintf(stderr, "border xp=%d yp=%d wp=%d hp=%d oxp=%d oyp=%d owp=%d ohp=%d\n",
		xp, yp, wp, hp,
		oxp, oyp, owp, ohp);
#endif

	if (!wp || !hp) return;

	if (xp < oxp) wp-=oxp-xp, xp=oxp;
	if (yp < oyp) hp-=oyp-yp, yp=oyp;

	if (xp+wp > oxp+owp) wp = oxp-xp+owp;
	if (yp+hp > oyp+ohp) hp = oyp-yp+ohp;

	buf = p_buf + (yp+p_yoff)*p_xsize + xp + p_xoff;

	for (j=0; j<hp; j++) {
		for(k=0 ; k < wp ; k++)
			buf[k] = 0x8000;
		//		memset(buf, 0, wp*sizeof(Pixel));
		buf += p_xsize;
	}
}

void 
redraw_phys_landscape(int xp, int yp, int wp, int hp)
{
	Pixel *src, *dst;
	int j;
	aa_line_func *aa_line = 0;
	int xv, yv;
	IMPORT_FRAMEBUFFER_VARS

	xv = v_x0 + xp*v_scale;
	yv = v_y0 + yp*v_scale;

	src = v_buf + yv*v_xsize + xv;
	dst = p_buf + (yp+p_yoff)*p_xsize + xp + p_xoff;

	switch(v_scale) {
	case 1: aa_line = s1aa_line; break;
	case 2: aa_line = s2aa_line; break;
	case 3: aa_line = s3aa_line; break;
	case 4: aa_line = s4aa_line; break;

	default:
		cleanup_and_exit("bad scale", EXIT_ERROR);
	}

	for (j=0; j<hp; j++) {
		aa_line(src, dst, wp);
		src += v_xsize * v_scale;
		dst += p_xsize;
	}
}

/*******************************************
 *        xp=0                  xp=p_xsize *
 *       +-----------------------------+   *
 *   yp=0|     xp             xp+wp    |   *
 *       |   yp+----------------+      |   *
 *       |     | v   v   v   v  |      |   *
 *       |     |j=0 j=1  v   v  |      |   *
 *       |     | v   v   v   v  |      |   *
 *       |     | v   v   v   v  |      |   *
 *       |     | v   v   v   v  |      |   *
 *       |     +----------------+      |   *
 *   yp= |                             |   *
 *p_ysize|                             |   *
 *       +-----------------------------+   *
 *                                         *
 *******************************************/
// TODO implemnt change pv_xsize/p_xoff...
void
redraw_phys_portrait(int xp, int yp, int wp, int hp)
{
	Pixel *src, *dst;
	int j;
	aa_line_func *aa_line = 0;
	int xv, yv;
	IMPORT_FRAMEBUFFER_VARS

#ifdef DEBUG_CLIP
	fprintf(stderr, "redraw_phys_portrait xp=%d yp=%d wp=%d hp=%d\n", xp, yp, wp, hp);
	fprintf(stderr, "v_x0=%d v_y0=%d\n", v_x0, v_y0);
#endif

	xv = v_x0 + yp*v_scale;
	yv = v_y0 + (p_xsize-xp-wp)*v_scale;

	src = v_buf + (yv+(wp-1)*v_scale)*v_xsize + xv;
	dst = p_buf + yp*p_xsize + xp;

#ifdef DEBUG_CLIP
	fprintf(stderr, "xv=%d yv=%d\n", xv, yv);
	fprintf(stderr, "v_buf=%p - %p, p_buf=%p - %p\n",
		v_buf, v_buf+v_xsize*v_ysize-1,
		p_buf, p_buf+p_xsize*p_ysize-1);
	fprintf(stderr, "src=%p dst=%p\n", src, dst);
#endif

	switch(v_scale) {
	case 1: aa_line = s1aa_line_portrait; break;
	case 2: aa_line = s2aa_line_portrait; break;
	case 3: aa_line = s3aa_line_portrait; break;
	case 4: aa_line = s4aa_line_portrait; break;

	default:
		cleanup_and_exit("bad scale", EXIT_ERROR);
	}

	if (v_scale==1) {
		for (j=0; j<wp; j+=2) {
			aa_line(src, dst, hp);
			src -= 2*v_xsize;
			dst += 2;
		}
	} else {
		for (j=0; j<wp; j++) {
			aa_line(src, dst, hp);
			src -= v_xsize*v_scale;
			dst += 1;
		}
	}
}

void
redraw_virt(int xv, int yv, int wv, int hv) {
	int xp, yp, hp, wp;
	IMPORT_FRAMEBUFFER_VARS

#ifdef DEBUG_CLIP
	fprintf(stderr, "redraw_virt: xv=%d yv=%d wv=%d hv=%d\n", xv, yv, wv, hv);
#endif

	if (v_scale == 1) {
		if (p_landscape) {
			xp = xv - v_x0;
			yp = yv - v_y0;
			wp = wv;
			hp = hv;
		} else {
			xp = pv_xsize - (yv + hv-1 - v_y0) - 1;
			yp = xv - v_x0;
			wp = hv;
			hp = wv;
		}
	} else {
		/* enlarge source to ensure that the rectangle edge lengths
		 * are evenly divisible by v_scale */
		int dx, dy;

		if (v_scale == 3 && p_landscape) {
			/* need additional pixels for subpixel antialiasing */
			xv -= 3;
			wv += 6;
		}
		
		dx = xv % v_scale;
		dy = yv % v_scale;

		xv -= dx;
		yv -= dy;
		wv += dx;
		hv += dy;

		if (p_landscape) {
			xp = (xv - v_x0) / v_scale;
			yp = (yv - v_y0) / v_scale;
			wp = (wv + v_scale-1) / v_scale;
			hp = (hv + v_scale-1) / v_scale;

			/* watch for right and bottom edge - might be 1 pixel 
			 * too large if size not divisible by scale */
			if (xv+v_scale*wp > v_xsize) wp--;
			if (yv+v_scale*hp > v_ysize) hp--;
		} else {
			xp = pv_xsize - ((yv + hv-1 - v_y0) / v_scale) - 1;
			yp = (xv - v_x0) / v_scale;
			wp = (hv + v_scale-1) / v_scale;
			hp = (wv + v_scale-1) / v_scale;

			if (xv+v_scale*hp > v_xsize) hp--;
			if (yv+v_scale*wp > v_ysize) wp--;
			
		}
	}

	/* clip to screen size */
	if (xp < 0) { wp += xp; xp = 0; }
	if (yp < 0) { hp += yp; yp = 0; }
	if (xp+wp > pv_xsize) wp = pv_xsize - xp;
	if (yp+hp > pv_ysize) hp = pv_ysize - yp;

	/* entirely off-screen ? */
	if (wp<=0 || hp<=0) return;

#ifdef DEBUG_CLIP
	fprintf(stderr, "\txp=%d yp=%d hp=%d wp=%d\n", xp, yp, hp, wp);
#endif

	if (p_landscape) {
		redraw_phys_landscape(xp, yp, wp, hp);
	} else {
		if (v_scale==1) {
			/* adjust coordinates, drawing is done in 2x2 blocks */
			if (xp&1) { xp--; wp++; }
			if (yp&1) { yp--; hp++; }
			if (wp&1) wp++;
			if (hp&1) hp++;
		}
		redraw_phys_portrait(xp, yp, wp, hp);
	}
	if (!hide_overlays) redraw_overlays(xp, yp, wp, hp);
}

void
redraw_phys(int xp, int yp, int wp, int hp) {
	int xv, yv;
	int oxp=xp, oyp=yp, owp=wp, ohp=hp;
	IMPORT_FRAMEBUFFER_VARS

#ifdef DEBUG_CLIP
	fprintf(stderr, "redraw_phys: xp=%d yp=%d wp=%d hp=%d\n", xp, yp, wp, hp);
#endif

	if (p_landscape) {
		xv = v_x0 + xp*v_scale;
		yv = v_y0 + yp*v_scale;

		if (yv<0) {
			yp = -v_y0/v_scale;
			yv = 0;
			draw_border(0, 0, pv_xsize, yp,
				oxp, oyp, owp, ohp);
		}
		if (yv+hp*v_scale > v_ysize) {
			hp = (v_ysize - yv)/v_scale;
			draw_border(0, yp+hp, pv_xsize, pv_ysize-yp-hp,
				oxp, oyp, owp, ohp);
		}
		if (xv<0) {
			xp = -v_x0/v_scale;
			xv = 0;
			draw_border(0, yp, xp, hp,
				oxp, oyp, owp, ohp);
		}
		if (xv+wp*v_scale > v_xsize) {
			wp = (v_xsize - xv)/v_scale;
			draw_border(xp+wp, yp, pv_xsize-xp-wp, hp,
				oxp, oyp, owp, ohp);
		}
	} else {
		xv = v_x0 + yp*v_scale;
		yv = v_y0 + (pv_xsize-xp-wp)*v_scale;

		if (xv<0) {
			yp = -v_x0/v_scale;
			xv = 0;
			draw_border(0, 0, pv_xsize, yp,
				oxp, oyp, owp, ohp);
		}
		if (xv+hp*v_scale > v_xsize) {
			hp = (v_xsize - xv)/v_scale;
			draw_border(0, yp+hp, pv_xsize, pv_ysize-yp-hp,
				oxp, oyp, owp, ohp);
		}
		if (yv<0) {
			int d = -yv / v_scale;
			xp -= d;
			yv = 0;
			draw_border(xp+wp, yp, pv_xsize-xp-wp, hp,
				oxp, oyp, owp, ohp);
		}
		if (yv+wp*v_scale > v_ysize) {
			int d = wp - (v_ysize - yv)/v_scale;
			wp = (v_ysize - yv)/v_scale;
			xp += d;
			draw_border(0, yp, xp, hp,
				oxp, oyp, owp, ohp);
		}
	}

#ifdef DEBUG_CLIP
	fprintf(stderr, "\tclip: xp=%d yp=%d wp=%d hp=%d\n", xp, yp, wp, hp);
#endif

	if (p_landscape) {
		redraw_phys_landscape(xp, yp, wp, hp);
	} else {
		if (v_scale==1) {
			/* adjust coordinates, drawing is done in 2x2 blocks */
			if (xp&1) { xp--; wp++; }
			if (yp&1) { yp--; hp++; }
			if (wp&1) wp++;
			if (hp&1) hp++;
		}
		redraw_phys_portrait(xp, yp, wp, hp);
	}
	if (!hide_overlays) redraw_overlays(oxp, oyp, owp, ohp);
}

void
redraw_phys_all(void)
{
	IMPORT_FRAMEBUFFER_VARS
	
	redraw_phys(0, 0, pv_xsize, pv_ysize);
	redraw_all_overlays();
}
