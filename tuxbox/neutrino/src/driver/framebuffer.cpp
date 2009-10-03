/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: framebuffer.cpp,v 1.79 2009/10/03 22:19:36 seife Exp $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
				  2003 thegoodguy

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/framebuffer.h>

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>

#include <linux/kd.h>

#include <stdint.h>
#include <cstdlib>
#ifdef HAVE_DBOX_HARDWARE
#include <dbox/fb.h>
#endif

#include <gui/color.h>
#include <gui/widget/icons.h>

#define BACKGROUNDIMAGEWIDTH 720

static uint8_t * virtual_fb = NULL;

CFrameBuffer::CFrameBuffer()
: active ( true )
{
	iconBasePath	= "";
	available		= 0;
	cmap.start		= 0;
	cmap.len		= 256;
	cmap.red		= red;
	cmap.green		= green;
	cmap.blue		= blue;
	cmap.transp		= trans;
	backgroundColor	= 0;
	background		= NULL;
	mute_save_bg		= NULL;
	mute_shown		= false;

	useBackgroundPaint	= false;
	backupBackground	= NULL;
	backgroundFilename	= "";

	fd	= 0;
	tty	= 0;
#ifdef HAVE_TRIPLEDRAGON
	gfxfd = -1;
#endif
}

CFrameBuffer* CFrameBuffer::getInstance()
{
	static CFrameBuffer* frameBuffer = NULL;

	if(!frameBuffer)
	{
		frameBuffer = new CFrameBuffer();
		printf("[neutrino] frameBuffer Instance created\n");
	}

	return frameBuffer;
}

void CFrameBuffer::init(const char * const fbDevice)
{
	fd = open(fbDevice, O_RDWR);
	if (fd<0)
	{
		perror(fbDevice);
		goto nolfb;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo)<0)
	{
		perror("FBIOGET_VSCREENINFO");
		goto nolfb;
	}

	memcpy(&oldscreen, &screeninfo, sizeof(screeninfo));

	fb_fix_screeninfo fix;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
		goto nolfb;
	}

	available=fix.smem_len;
	printf("%dk video mem\n", available/1024);
	lfb=(fb_pixel_t*)mmap(0, available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

	if (!lfb)
	{
		perror("mmap");
		goto nolfb;
	}

	if ((tty=open("/dev/vc/0", O_RDWR))<0)
	{
		perror("open (tty)");
		goto nolfb;
	}

	struct sigaction act;

	memset(&act,0,sizeof(act));
	act.sa_handler  = switch_signal;
	sigemptyset(&act.sa_mask);
	sigaction(SIGUSR1,&act,NULL);
	sigaction(SIGUSR2,&act,NULL);

	struct vt_mode mode;

	if (-1 == ioctl(tty,KDGETMODE, &kd_mode)) {
		perror("ioctl KDGETMODE");
		goto nolfb;
	}

	if (-1 == ioctl(tty,VT_GETMODE, &vt_mode)) {
			perror("ioctl VT_GETMODE");
		goto nolfb;
	}

	if (-1 == ioctl(tty,VT_GETMODE, &mode)) {
			perror("ioctl VT_GETMODE");
		goto nolfb;
	}

	mode.mode   = VT_PROCESS;
	mode.waitv  = 0;
	mode.relsig = SIGUSR1;
	mode.acqsig = SIGUSR2;

	if (-1 == ioctl(tty,VT_SETMODE, &mode)) {
		perror("ioctl VT_SETMODE");
		goto nolfb;
	}

	if (-1 == ioctl(tty,KDSETMODE, KD_GRAPHICS)) {
		perror("ioctl KDSETMODE");
		goto nolfb;
	}

	return;

nolfb:
	printf("framebuffer not available.\n");
	lfb=0;
}


CFrameBuffer::~CFrameBuffer()
{
	if (background)
	{
		delete[] background;
	}

	if (mute_save_bg)
		delete[] mute_save_bg;

	if (backupBackground)
	{
		delete[] backupBackground;
	}

#ifdef HAVE_TRIPLEDRAGON
	if (gfxfd > -1)
	{
		if (ioctl(gfxfd, STB04GFX_OSD_SETCONTROL, &gfxctrl) < 0)
			fprintf(stderr, "[Framebuffer] STB04GFX_OSD_SETCONTROL failed: %m\n");
		close(gfxfd);
		gfxfd = -1;
	}
#endif
#ifdef RETURN_FROM_GRAPHICS_MODE
	if (-1 == ioctl(tty,KDSETMODE, kd_mode))
		perror("ioctl KDSETMODE");
#endif

	if (-1 == ioctl(tty,VT_SETMODE, &vt_mode))
		perror("ioctl VT_SETMODE");

	/*
	if (available)
		ioctl(fd, FBIOPUT_VSCREENINFO, &oldscreen);
	if (lfb)
		munmap(lfb, available);
		*/
	
	if (virtual_fb)
		delete[] virtual_fb;
}

#ifdef HAVE_TRIPLEDRAGON
void CFrameBuffer::makeTransparent()
{
	// fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
	Stb04GFXOsdControl tmpctrl;
	if (gfxfd < 0)
	{
		gfxfd = open("/dev/stb/tdgfx", O_RDONLY);
		if (ioctl(gfxfd, STB04GFX_OSD_ONOFF, 1) < 0)
			fprintf(stderr, "[CFB::%s] STB04GFX_OSD_ONOFF failed: %m\n", __FUNCTION__);
		if (ioctl(gfxfd, STB04GFX_OSD_GETCONTROL, &gfxctrl) < 0)
			fprintf(stderr, "[CFB::%s:%d] STB04GFX_OSD_GETCONTROL failed: %m\n", __FUNCTION__, __LINE__);
		fprintf(stderr, "FB: x: %d y: %d w: %d h: %d off: %d depth: %d ga: %d use_ga: %d ff: %d 169ad: %d sqp: %d gc: %d undef: %d\n",
			gfxctrl.x,
			gfxctrl.y,
			gfxctrl.width,
			gfxctrl.height,
			gfxctrl.offset,
			gfxctrl.depth,
			(int)gfxctrl.global_alpha,
			gfxctrl.use_global_alpha,
			gfxctrl.enable_flicker_filter,
			gfxctrl.enable_16_9_adjust,
			gfxctrl.enable_square_pixel_filter,
			gfxctrl.enable_gamma_correction,
			gfxctrl.undefined_Colors_Transparent);
		memcpy(&tmpctrl, &gfxctrl, sizeof(tmpctrl));
#if 0
//defaults after boot:
//[zapit.cpp:main:2991] x: 0 y: 0 w: 720 h: 576 off: 0 depth: 32 ga: 176 use_ga: 1 ff: 1 169ad: 0 sqp: 0 gc: 0 undef: 0
	gfxctrl.use_global_alpha = 1;
	gfxctrl.global_alpha=176; //default
	gfxctrl.enable_gamma_correction = 1;
#endif
	/* this is needed, otherwise the picture will be shaded
	   unfortunately, this is all pretty much undocumented... */
	}
	else
	{
		if (ioctl(gfxfd, STB04GFX_OSD_GETCONTROL, &tmpctrl) < 0)
			fprintf(stderr, "[CFB::%s:%d] STB04GFX_OSD_GETCONTROL failed: %m\n", __FUNCTION__, __LINE__);
	}
	/* if "use_global_alpha = 1" then the transparency of the framebuffer does not work.
	   but we can set the global transparency. global transparency means that there is
	   no individual setting, so we don't use it. */
	tmpctrl.use_global_alpha = 0;
	tmpctrl.undefined_Colors_Transparent = 1;
	if (ioctl(gfxfd, STB04GFX_OSD_SETCONTROL, &tmpctrl) < 0)
		fprintf(stderr, "[CFB::%s] STB04GFX_OSD_SETCONTROL failed: %m\n", __FUNCTION__);
}
#endif

int CFrameBuffer::getFileHandle() const
{
	return fd;
}

unsigned int CFrameBuffer::getStride() const
{
	return stride;
}

fb_pixel_t * CFrameBuffer::getFrameBufferPointer() const
{
	if (active || (virtual_fb == NULL))
		return lfb;
	else
		return (fb_pixel_t *) virtual_fb;
}

bool CFrameBuffer::getActive() const
{
	return (active || (virtual_fb != NULL));
}

bool CFrameBuffer::getActiveReally() const
{
	return active;
}

t_fb_var_screeninfo *CFrameBuffer::getScreenInfo()
{
	return &screeninfo;
}

int CFrameBuffer::setMode(unsigned int nxRes, unsigned int nyRes, unsigned int nbpp)
{
	if (!available&&!active)
		return -1;

	screeninfo.xres_virtual=screeninfo.xres=nxRes;
	screeninfo.yres_virtual=screeninfo.yres=nyRes;
	screeninfo.bits_per_pixel=nbpp;

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo)<0)
	{
		perror("FBIOPUT_VSCREENINFO");
#if 0
		return -1;
#endif
	}

	if ((screeninfo.xres!=nxRes) && (screeninfo.yres!=nyRes) && (screeninfo.bits_per_pixel!=nbpp))
	{
		printf("SetMode failed: wanted: %dx%dx%d, got %dx%dx%d\n",
		       nxRes, nyRes, nbpp,
		       screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);
		return -1;
	}

	xRes = screeninfo.xres;
	yRes = screeninfo.yres;
	bpp  = screeninfo.bits_per_pixel;
	fb_fix_screeninfo fix;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
		return -1;
	}

	stride=fix.line_length;
	memset(getFrameBufferPointer(), 0, stride * yRes);
	return 0;
}


void CFrameBuffer::paletteFade(int i, __u32 rgb1, __u32 rgb2, int level)
{
	__u16 *r = cmap.red+i;
	__u16 *g = cmap.green+i;
	__u16 *b = cmap.blue+i;
	*r= ((rgb2&0xFF0000)>>16)*level;
	*g= ((rgb2&0x00FF00)>>8 )*level;
	*b= ((rgb2&0x0000FF)    )*level;
	*r+=((rgb1&0xFF0000)>>16)*(255-level);
	*g+=((rgb1&0x00FF00)>>8 )*(255-level);
	*b+=((rgb1&0x0000FF)    )*(255-level);
}

#if defined HAVE_DBOX_HARDWARE
void CFrameBuffer::setTransparency( int tr )
{
	if (!active)
		return;

	if (tr> 8)
		tr= 8;

	int val = (tr << 8) | tr;
	if (ioctl(fd, AVIA_GT_GV_SET_BLEV, val ))
		perror("AVIA_GT_GV_SET_BLEV");

}

void CFrameBuffer::setBlendLevel(int blev1, int blev2)
{
	unsigned int c;

	c=(blev2 & 0x0F) <<8 | (blev1 & 0x0F);

	if (ioctl(fd,AVIA_GT_GV_SET_BLEV, c) < 0)
		perror("AVIA_GT_GV_SET_BLEV:");
}
#endif

#ifdef FB_USE_PALETTE
void CFrameBuffer::setAlphaFade(int in, int num, int tr)
{
	for (int i=0; i<num; i++)
	{
		cmap.transp[in+i]=tr;
		//tr++;
	}
}
#else
/* does not work in non-palette mode */
void CFrameBuffer::setAlphaFade(int, int, int)
{
}
#endif

void CFrameBuffer::paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr)
{
	for (int i=0; i<num; i++)
	{
		paletteFade(in+i, rgb1, rgb2, i*(255/(num-1)));
		cmap.transp[in+i]=tr;
		tr++;
	}
}

void CFrameBuffer::paletteSetColor(int i, __u32 rgb, int tr)
{
	cmap.red[i]    =(rgb&0xFF0000)>>8;
	cmap.green[i]  =(rgb&0x00FF00)   ;
	cmap.blue[i]   =(rgb&0x0000FF)<<8;
	cmap.transp[i] =tr;
}

#ifndef FB_USE_PALETTE
inline fb_pixel_t make16color(uint16_t r, uint16_t g, uint16_t b, uint16_t t,
				  uint32_t rl, uint32_t ro,
				  uint32_t gl, uint32_t go,
				  uint32_t bl, uint32_t bo,
				  uint32_t tl, uint32_t to)
{
	return (
		((t >> (16 - tl)) << to) |
		((r >> (16 - rl)) << ro) |
		((g >> (16 - gl)) << go) |
		((b >> (16 - bl)) << bo));
}
#endif
void CFrameBuffer::paletteSet(struct fb_cmap *map)
{
	if (!active)
		return;
	
	if(map == NULL)
		map = &cmap;

#ifdef FB_USE_PALETTE
	ioctl(fd, FBIOPUTCMAP, map);
#else
	uint32_t rl, ro, gl, go, bl, bo, tl, to;
	
	rl = screeninfo.red.length;
	ro = screeninfo.red.offset;
	gl = screeninfo.green.length;
	go = screeninfo.green.offset;
	bl = screeninfo.blue.length;
	bo = screeninfo.blue.offset;
	tl = screeninfo.transp.length;
	to = screeninfo.transp.offset;

	for (int i = 0; i < 256; i++)
	{
		realcolor[i] = make16color(cmap.red[i], cmap.green[i], cmap.blue[i], cmap.transp[i],
					   rl, ro, gl, go, bl, bo, tl, to);
	}
#endif
#ifdef HAVE_TRIPLEDRAGON
	makeTransparent();
#endif
}

void CFrameBuffer::paintVLineRel(int x, int y, int dy, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;

	for(int count=0;count<dy;count++)
	{
		*(fb_pixel_t *)pos = col;
		pos += stride;
	}
}

void CFrameBuffer::paintHLineRel(int x, int dx, int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;

#ifdef FB_USE_PALETTE
	memset(pos, col, dx);
//	memset(pos, col, dx * sizeof(fb_pixel_t));
#else
	fb_pixel_t * dest = (fb_pixel_t *)pos;
	for (int i = 0; i < dx; i++)
		*(dest++) = col;
#endif
}

void CFrameBuffer::setIconBasePath(const std::string & iconPath)
{
	iconBasePath = iconPath;
}

std::string CFrameBuffer::getIconFilePath(const std::string & filename)
/*    	
 *  	filename can be a single filename eg. "<filename>" 
 *  	or absolute path eg. "/var/dir/<filename>" 
 */
{	
	std::string 	res,
						defaultIconPath = iconBasePath + filename,
						alterIconPath = (access(filename.c_str(), 0 ) != -1) ? filename : (std::string)NEUTRINO_ICON_VARPATH + filename;

	if ((access(alterIconPath.c_str(), 0 ) != -1))	{
			res = alterIconPath;
		}
	else
		res = defaultIconPath;

	return res;
}

int CFrameBuffer::getIconHeight(const char * const filename)
{
	struct rawHeader header;
	uint16_t         height;
	int              icon_fd;
	std::string      iconfile = getIconFilePath(filename);

	icon_fd = open(iconfile.c_str(), O_RDONLY);

	if (icon_fd == -1)
	{
		printf("Framebuffer getIconHeight: error while loading icon: %s\n", iconfile.c_str());
		return 0;
	}
	else
	{	
		read(icon_fd, &header, sizeof(struct rawHeader));
		height = (header.height_hi << 8) | header.height_lo;
	}

	close(icon_fd);
	return height;
}

int CFrameBuffer::getIconWidth(const char * const filename)
{
	struct rawHeader header;
	uint16_t         width;
	int              icon_fd;
	std::string      iconfile = getIconFilePath(filename);

	icon_fd = open(iconfile.c_str(), O_RDONLY);

	if (icon_fd == -1)
	{
		printf("Framebuffer getIconWidth: error while loading icon: %s\n", iconfile.c_str());
		width = 0;
	}
	else
	{	
		read(icon_fd, &header, sizeof(struct rawHeader));
		width = (header.width_hi << 8) | header.width_lo;
	}

	close(icon_fd);
	return width;
}

bool CFrameBuffer::paintIcon8(const std::string & filename, const int x, const int y, const unsigned char offset)
{
	if (!getActive())
		return false;

	struct rawHeader header;
	uint16_t         width, height;
	int              icon_fd;
	std::string      iconfile = getIconFilePath(filename);

	icon_fd = open(iconfile.c_str(), O_RDONLY);

	if (icon_fd == -1)
	{
		printf("Framebuffer paintIcon8: error while loading icon: %s\n", iconfile.c_str());
		return false;
	}
	read(icon_fd, &header, sizeof(struct rawHeader));

	width  = (header.width_hi  << 8) | header.width_lo;
	height = (header.height_hi << 8) | header.height_lo;

	bool muted = checkMute(x, width, y, height);
	if (muted)
		paintMuteIcon(false, mute_x, mute_y);

	unsigned char pixbuf[768];

	uint8_t * d = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
	fb_pixel_t * d2;
	for (int count=0; count<height; count ++ )
	{
		read(icon_fd, &pixbuf, width );
		unsigned char *pixpos = (unsigned char*) &pixbuf;
		d2 = (fb_pixel_t *) d;
		for (int count2=0; count2<width; count2 ++ )
		{
			unsigned char color = *pixpos;
			if (color != header.transp)
			{
				paintPixel(d2, color + offset);
			}
			d2++;
			pixpos++;
		}
		d += stride;
	}
	close(icon_fd);

	if (muted)
		paintMuteIcon(true, mute_x, mute_y);

	return true;
}	

bool CFrameBuffer::paintIcon(const std::string & filename, const int x, const int y, const unsigned char offset)
{
	if (!getActive())
		return false;

	struct rawHeader header;
	uint16_t         width, height;
	int              icon_fd;
	std::string      iconfile = getIconFilePath(filename);

	icon_fd = open(iconfile.c_str(), O_RDONLY);

	if (icon_fd == -1)
	{
		printf("Framebuffer paintIcon: error while loading icon: %s\n", iconfile.c_str());
		return false;
	}

	read(icon_fd, &header, sizeof(struct rawHeader));

	width  = (header.width_hi  << 8) | header.width_lo;
	height = (header.height_hi << 8) | header.height_lo;

	bool muted = checkMute(x, width, y, height);
	if (muted)
		paintMuteIcon(false, mute_x, mute_y);

	unsigned char pixbuf[768];
	uint8_t * d = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
	fb_pixel_t * d2;
	for (int count=0; count<height; count ++ )
	{
		read(icon_fd, &pixbuf, width >> 1 );
		unsigned char *pixpos = (unsigned char*) &pixbuf;
		d2 = (fb_pixel_t *) d;
		for (int count2=0; count2<width >> 1; count2 ++ )
		{
			unsigned char compressed = *pixpos;
			unsigned char pix1 = (compressed & 0xf0) >> 4;
			unsigned char pix2 = (compressed & 0x0f);

			if (pix1 != header.transp)
			{
				paintPixel(d2, pix1 + offset);
			}
			d2++;
			if (pix2 != header.transp)
			{
				paintPixel(d2, pix2 + offset);
			}
			d2++;
			pixpos++;
		}
		d += stride;
	}

	if (muted)
		paintMuteIcon(true, mute_x, mute_y);

	close(icon_fd);
	return true;
}

bool CFrameBuffer::paintIcon(const char * const filename, const int x, const int y, const unsigned char offset)
{
	return paintIcon(std::string(filename), x, y, offset);
}

void CFrameBuffer::loadPal(const std::string & filename, const unsigned char offset, const unsigned char endidx)
{
	if (!getActive())
		return;

	struct rgbData rgbdata;
	int            pal_fd;
	std::string  palfile = getIconFilePath(filename);

	pal_fd = open(palfile.c_str(), O_RDONLY);

	if (pal_fd == -1)
	{
		printf("error while loading palette: %s\n", palfile.c_str());
		return;
	}

	int pos = 0;
	int readb = read(pal_fd, &rgbdata,  sizeof(rgbdata) );
	while(readb)
	{
		__u32 rgb = (rgbdata.r<<16) | (rgbdata.g<<8) | (rgbdata.b);
		int colpos = offset+pos;
		if( colpos>endidx)
			break;
		paletteSetColor(colpos, rgb ,0);
		readb = read(pal_fd, &rgbdata,  sizeof(rgbdata) );
		pos++;
	}
	paletteSet(&cmap);
	close(pal_fd);
}

void CFrameBuffer::paintPixel(const int x, const int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	fb_pixel_t * pos = getFrameBufferPointer();
	pos += (stride / sizeof(fb_pixel_t)) * y;
	pos += x;

	*pos = col;
}

void CFrameBuffer::paintBoxRel(const int x, const int y, const int dx, const int dy, const fb_pixel_t col, const int radius, const int corners)
{
	int F,R=radius,sx,sy,dxx=dx,dyy=dy,rx,ry,wx,wy;
	int corner1 =  corners     & 1;	// upper left
	int corner2 = (corners>>1) & 1;	// upper right
	int corner3 = (corners>>2) & 1;	// lower right
	int corner4 = (corners>>3) & 1;	// lower left
	int line = stride / sizeof(fb_pixel_t);

	if (!getActive())
		return;

	/* helpful for debugging problems with the color definitions */
	//assert (!col || col > 0xff);

	fb_pixel_t *pos = getFrameBufferPointer() + x + y * line;
	fb_pixel_t *pos0, *pos1, *pos2, *pos3;

#ifdef FB_USE_PALETTE
	if (dxx<0) {
		fprintf(stderr, "ERROR: CFrameBuffer::paintBoxRel called with dx < 0 (%d)\n", dxx);
		dxx=0;
	}
#else
	fb_pixel_t *dest0, *dest1;
#endif

	bool muted = checkMute(x, dx, y, dy);
	if (muted)
		paintMuteIcon(false, mute_x, mute_y);

	if(R)
	{
		if(--dyy<=0)
		{
			dyy=1;
		}

		if(R==1 || R>(dxx/2) || R>(dyy/2))
		{
			R=dxx/10;
			F=dyy/10;    
			if(R>F)
			{
				if(R>(dyy/3))
				{
					R=dyy/3;
				}
			}
			else
			{
				R=F;
				if(R>(dxx/3))
				{
					R=dxx/3;
				}
			}
		}

		sx=0;
		sy=R;
		F=1-R;
		rx=R-sx;
		ry=R-sy;
		pos0 = pos + (dyy - ry) * line;
		pos1 = pos + ry * line;
		pos2 = pos + rx * line;
		pos3 = pos + (dyy-rx) * line;

		while (sx <= sy)
		{
			rx=R-sx;
			ry=R-sy;
			wx=rx<<1;
			wy=ry<<1;
#ifdef FB_USE_PALETTE
			memset(pos0+(rx*corner4), col, dxx-(wx*corner3)-(rx*(1-corner3))+(rx*(1-corner4)));
			memset(pos1+(rx*corner1), col, dxx-(wx*corner2)-(rx*(1-corner2))+(rx*(1-corner1)));
			memset(pos2+(ry*corner1), col, dxx-(wy*corner2)-(ry*(1-corner2))+(ry*(1-corner1)));
			memset(pos3+(ry*corner4), col, dxx-(wy*corner3)-(ry*(1-corner3))+(ry*(1-corner4)));
#else
			dest0=(fb_pixel_t *)(pos0+(rx*corner4));
			for (int i=0; i<(dxx-(wx*corner3)-(rx*(1-corner3))+(rx*(1-corner4))); i++)
				*(dest0++)=col;

			dest1=(fb_pixel_t *)(pos1+(rx*corner1));
			for (int i=0; i<(dxx-(wx*corner2)-(rx*(1-corner2))+(rx*(1-corner1))); i++)
				*(dest1++)=col;

			dest0=(fb_pixel_t *)(pos2+(ry*corner1));
			for (int i=0; i<(dxx-(wy*corner2)-(ry*(1-corner2))+(ry*(1-corner1))); i++)
				*(dest0++)=col;

			dest1=(fb_pixel_t *)(pos3+(ry*corner4));
			for (int i=0; i<(dxx-(wy*corner3)-(ry*(1-corner3))+(ry*(1-corner4))); i++)
				*(dest1++)=col;

#endif
			sx++;
			pos2 -= line;
			pos3 += line;
			if (F<0)
			{
				F+=(sx<<1)-1;
			}
			else
			{
				F+=((sx-sy)<<1);
				sy--;
				pos0 -= line;
				pos1 += line;
			}
		}
		pos += R * line;
	}

	for (int count=R; count<(dyy-R); count++)
	{
#ifdef FB_USE_PALETTE
		memset(pos, col, dxx);
#else
		dest0=(fb_pixel_t *)pos;
		for (int i=0; i<dxx; i++)
			*(dest0++)=col;
#endif
		pos += line;
	}

	if (muted)
		paintMuteIcon(true, mute_x, mute_y);
}

void CFrameBuffer::paintLine(int xa, int ya, int xb, int yb, const fb_pixel_t col)
{
	if (!getActive())
		return;

	int dx = abs (xa - xb);
	int dy = abs (ya - yb);
	int x;
	int y;
	int End;
	int step;

	if ( dx > dy )
	{
		int	p = 2 * dy - dx;
		int	twoDy = 2 * dy;
		int	twoDyDx = 2 * (dy-dx);

		if ( xa > xb )
		{
			x = xb;
			y = yb;
			End = xa;
			step = ya < yb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = xb;
			step = yb < ya ? -1 : 1;
		}

		paintPixel (x, y, col);

		while( x < End )
		{
			x++;
			if ( p < 0 )
				p += twoDy;
			else
			{
				y += step;
				p += twoDyDx;
			}
			paintPixel (x, y, col);
		}
	}
	else
	{
		int	p = 2 * dx - dy;
		int	twoDx = 2 * dx;
		int	twoDxDy = 2 * (dx-dy);

		if ( ya > yb )
		{
			x = xb;
			y = yb;
			End = ya;
			step = xa < xb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = yb;
			step = xb < xa ? -1 : 1;
		}

		paintPixel (x, y, col);

		while( y < End )
		{
			y++;
			if ( p < 0 )
				p += twoDx;
			else
			{
				x += step;
				p += twoDxDy;
			}
			paintPixel (x, y, col);
		}
	}
}

void CFrameBuffer::setBackgroundColor(const fb_pixel_t color)
{
	backgroundColor = color;
}

bool CFrameBuffer::loadPictureToMem(const std::string & filename, const uint16_t width, const uint16_t height, const uint16_t _stride, fb_pixel_t * memp)
{
	struct rawHeader header;
	int              pic_fd;

	std::string  picturefile = getIconFilePath(filename);
	
	pic_fd = open(picturefile.c_str(), O_RDONLY );

	if (pic_fd == -1)
	{
		printf("error while loading icon: %s\n", picturefile.c_str());
		return false;
	}

	read(pic_fd, &header, sizeof(struct rawHeader));

	if ((width  != ((header.width_hi  << 8) | header.width_lo)) ||
	    (height != ((header.height_hi << 8) | header.height_lo)))
	{
		printf("error while loading icon: %s - invalid resolution = %hux%hu\n", filename.c_str(), width, height);
		return false;
	}

	if ((_stride == 0) || (_stride == width * sizeof(fb_pixel_t)))
		read(pic_fd, memp, height * width * sizeof(fb_pixel_t));
	else
		for (int i = 0; i < height; i++)
			read(pic_fd, ((uint8_t *)memp) + i * _stride, width * sizeof(fb_pixel_t));

	close(pic_fd);
	return true;
}

bool CFrameBuffer::loadPicture2Mem(const std::string & filename, fb_pixel_t * memp)
{
	return loadPictureToMem(filename, BACKGROUNDIMAGEWIDTH, 576, 0, memp);
}

bool CFrameBuffer::loadPicture2FrameBuffer(const std::string & filename)
{
	if (!getActive())
		return false;

	return loadPictureToMem(filename, BACKGROUNDIMAGEWIDTH, 576, getStride(), getFrameBufferPointer());
}

bool CFrameBuffer::savePictureFromMem(const std::string & filename, const fb_pixel_t * const memp)
{
	struct rawHeader header;
	uint16_t         width, height;
	int              pic_fd;
	
	width = BACKGROUNDIMAGEWIDTH;
	height = 576;

	header.width_lo  = width  &  0xFF;
	header.width_hi  = width  >>    8;
	header.height_lo = height &  0xFF;
	header.height_hi = height >>    8;
	header.transp    =              0;
	
	std::string picturefile = getIconFilePath(filename);

	pic_fd = open(picturefile.c_str(), O_WRONLY | O_CREAT);

	if (pic_fd==-1)
	{
		printf("error while saving icon: %s", picturefile.c_str() );
		return false;
	}

	write(pic_fd, &header, sizeof(struct rawHeader));

	write(pic_fd, memp, width * height * sizeof(fb_pixel_t));

	close(pic_fd);
	return true;
}

bool CFrameBuffer::loadBackground(const std::string & filename, const unsigned char offset)
{
	std::string picturefile = getIconFilePath(filename);

	if ((backgroundFilename == picturefile) && (background))
	{
		// loaded previously
		return true;
	}

	if (background)
	{
		delete[] background;
	}

	background = new fb_pixel_t[BACKGROUNDIMAGEWIDTH * 576];

	if (!loadPictureToMem(picturefile, BACKGROUNDIMAGEWIDTH, 576, 0, background))
	{
		delete[] background;
		background=0;
		return false;
	}

	if (offset != 0)//pic-offset
	{
		fb_pixel_t * bpos = background;
		int pos = BACKGROUNDIMAGEWIDTH * 576;
		while (pos > 0)
		{
			*bpos += offset;
			bpos++;
			pos--;
		}
	}

#ifndef FB_USE_PALETTE
	fb_pixel_t * dest = background + BACKGROUNDIMAGEWIDTH * 576;
	uint8_t    * src  = ((uint8_t * )background)+ BACKGROUNDIMAGEWIDTH * 576;
	for (int i = 576 - 1; i >= 0; i--)
		for (int j = BACKGROUNDIMAGEWIDTH - 1; j >= 0; j--)
		{
			dest--;
			src--;
			paintPixel(dest, *src);
		}
#endif

	backgroundFilename = picturefile;

	return true;
}

void CFrameBuffer::useBackground(bool ub)
{
	useBackgroundPaint = ub;
}

bool CFrameBuffer::getuseBackground(void)
{
	return useBackgroundPaint;
}

void CFrameBuffer::saveBackgroundImage(void)
{
	if (backupBackground != NULL)
		delete[] backupBackground;

	backupBackground = background;
	useBackground(false); // <- necessary since no background is available
	background = NULL;
}

void CFrameBuffer::restoreBackgroundImage(void)
{
	fb_pixel_t * tmp = background;

	if (backupBackground != NULL)
	{
		background = backupBackground;
		backupBackground = NULL;
	}
	else
		useBackground(false); // <- necessary since no background is available

	if (tmp != NULL)
		delete[] tmp;
}

void CFrameBuffer::paintBackgroundBoxRel(int x, int y, int dx, int dy)
{
	if (!getActive())
		return;

	if (x + dx > xRes)
	{
		fprintf(stderr, "%s:%d invalid x (%d), dx (%d), sum = %d > xRes (%d)\n", __FUNCTION__, __LINE__, x, dx, x+dx, xRes);
		dx = xRes - x;
	}
	if (y + dy > yRes)
	{
		fprintf(stderr, "%s:%d invalid y (%d), dy (%d), sum = %d > yRes (%d)\n", __FUNCTION__, __LINE__, y, dy, y+dy, yRes);
		dy = yRes - y;
	}

	if(!useBackgroundPaint)
	{
		paintBoxRel(x, y, dx, dy, backgroundColor);
	}
	else
	{
		bool muted = checkMute(x, dx, y, dy);
		if (muted)
			paintMuteIcon(false, mute_x, mute_y);

		uint8_t * fbpos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
		fb_pixel_t * bkpos = background + x + BACKGROUNDIMAGEWIDTH * y;
		for(int count = 0;count < dy; count++)
		{
			memcpy(fbpos, bkpos, dx * sizeof(fb_pixel_t));
			fbpos += stride;
			bkpos += BACKGROUNDIMAGEWIDTH;
		}
		if (muted)
			paintMuteIcon(true, mute_x, mute_y);
	}
}

void CFrameBuffer::paintBackground()
{
	if (!getActive())
		return;

	bool muted = mute_shown; // no need to check region...
	if (muted)
		paintMuteIcon(false, mute_x, mute_y);

	if (useBackgroundPaint && (background != NULL))
	{
		for (int i = 0; i < 576; i++)
			memcpy(((uint8_t *)getFrameBufferPointer()) + i * stride, (background + i * BACKGROUNDIMAGEWIDTH), BACKGROUNDIMAGEWIDTH * sizeof(fb_pixel_t));
	}
	else
	{
#ifdef FB_USE_PALETTE
		memset(getFrameBufferPointer(), backgroundColor, stride * 576);
#else
		paintBoxRel(0, 0, BACKGROUNDIMAGEWIDTH, 576, backgroundColor);
#endif
	}

	if (muted)
		paintMuteIcon(true, mute_x, mute_y);
}

void CFrameBuffer::SaveScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp)
{
	if (!getActive())
		return;

	bool muted = checkMute(x, dx, y, dy);
	if (muted)
		paintMuteIcon(false, mute_x, mute_y);

	uint8_t * fbpos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
	fb_pixel_t * bkpos = memp;
	for (int count = 0; count < dy; count++)
	{
		memcpy(bkpos, fbpos, dx * sizeof(fb_pixel_t));
		fbpos += stride;
		bkpos += dx;
	}

	if (muted)
		paintMuteIcon(true, mute_x, mute_y);
}

void CFrameBuffer::RestoreScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp)
{
	if (!getActive())
		return;

	bool muted = checkMute(x, dx, y, dy);
	if (muted)
		paintMuteIcon(false, mute_x, mute_y);

	uint8_t * fbpos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
	fb_pixel_t * bkpos = memp;
	for (int count = 0; count < dy; count++)
	{
		memcpy(fbpos, bkpos, dx * sizeof(fb_pixel_t));
		fbpos += stride;
		bkpos += dx;
	}

	if (muted)
		paintMuteIcon(true, mute_x, mute_y);
}

void CFrameBuffer::switch_signal (int signal)
{
	CFrameBuffer * thiz = CFrameBuffer::getInstance();
	if (signal == SIGUSR1) {
		if (virtual_fb != NULL)
			delete[] virtual_fb;
		virtual_fb = new uint8_t[thiz->stride * thiz->yRes];
		thiz->active = false;
		if (virtual_fb != NULL)
			memcpy(virtual_fb, thiz->lfb, thiz->stride * thiz->yRes);
		ioctl(thiz->tty, VT_RELDISP, 1);
		printf ("release display\n");
	}
	else if (signal == SIGUSR2) {
		ioctl(thiz->tty, VT_RELDISP, VT_ACKACQ);
		thiz->active = true;
		printf ("acquire display\n");
		thiz->paletteSet(NULL);
		if (virtual_fb != NULL)
			memcpy(thiz->lfb, virtual_fb, thiz->stride * thiz->yRes);
		else
			memset(thiz->lfb, 0, thiz->stride * thiz->yRes);
	}
}

void CFrameBuffer::setScreenSize(int sx, int sy, int ex, int ey)
{
	screen_StartX = sx;
	screen_StartY = sy;
	screen_EndX = ex;
	screen_EndY = ey;
}

// returns if area overlaps mute icon
bool CFrameBuffer::checkMute(int xs, int dx, int ys, int dy)
{
	if (! mute_shown)
		return false;
	if (ys > mute_y + 40)
		return false;
	if (xs + dx < mute_x)
		return false;
	if (xs > mute_x + 40)
		return false;
	if (ys + dy < mute_y)
		return false;

	// fprintf(stderr, "%s xs: %d dx: %d ys: %d dy: %d m_x: %d m_y: %d\n", __FUNCTION__, xs, dx, ys, dy, mute_x, mute_y);
	return true;
}

void CFrameBuffer::paintMuteIcon(bool is_visible, int _x, int _y)
{
	if (_x < 0)
		mute_x = screen_EndX - MUTE_WIDTH;
	else
		mute_x = _x;
	if (_y < 0)
		mute_y = screen_StartY;
	else
		mute_y = _y;

	// fprintf(stderr, "%s: visible: %d mute_shown: %d\n", __FUNCTION__, is_visible, mute_shown);
	if (mute_shown == is_visible) // nothing to do.
			return;
	if (is_visible)
	{
		if (! mute_save_bg)
			mute_save_bg = new fb_pixel_t[MUTE_WIDTH * MUTE_HEIGHT];
		if (mute_save_bg)
			SaveScreen(mute_x, mute_y, MUTE_WIDTH, MUTE_HEIGHT, mute_save_bg);

		paintBoxRel(mute_x, mute_y, MUTE_WIDTH, MUTE_HEIGHT, COL_INFOBAR_PLUS_0, 12); // 12 = RADIUS_LARGE
		paintIcon(NEUTRINO_ICON_BUTTON_MUTE, mute_x + 4, mute_y + 4);
		mute_shown = true;
	}
	else
	{
		mute_shown = false;
		if (mute_save_bg)
			RestoreScreen(mute_x, mute_y, MUTE_WIDTH, MUTE_HEIGHT, mute_save_bg);
		else
			paintBackgroundBoxRel(mute_x, mute_y, MUTE_WIDTH, MUTE_HEIGHT);
	}
}

void CFrameBuffer::ClearFrameBuffer()
{
#ifdef FB_USE_PALETTE
	setBackgroundColor(COL_BACKGROUND);
#endif
	useBackground(false);

	paintBackground();

	//background
	paletteSetColor(COL_BACKGROUND, 0x000000, 0xffff);

	//Windows Colors
	paletteSetColor(0x1, 0x010101, 0); 
	paletteSetColor(COL_MAROON, 0x800000, 0); 
	paletteSetColor(COL_GREEN, 0x008000, 0); 
	paletteSetColor(COL_OLIVE, 0x808000, 0); 
	paletteSetColor(COL_NAVY, 0x000080, 0); 
	paletteSetColor(COL_PURPLE, 0x800080, 0); 
	paletteSetColor(COL_TEAL, 0x008080, 0); 
	
	//paletteSetColor(COL_SILVER, 0xC0C0C0, 0); //silver --> 0xC0C0C0 is the original VGA colornumber
	paletteSetColor(COL_SILVER, 0xA0A0A0, 0); //silver --> 0xA0A0A0 is only a compromise!

	//paletteSetColor(COL_GRAY, 0x808080, 0); //gray --> 0x808080 is is the original VGA colornumber
	paletteSetColor(COL_GRAY, 0x505050, 0); //gray --> 0x505050 is only a compromise

	paletteSetColor(COL_RED, 0xFF0000, 0); 
	paletteSetColor(COL_LIME, 0x00FF00, 0); 
	paletteSetColor(COL_YELLOW, 0xFFFF00, 0); 
	paletteSetColor(COL_BLUE, 0x0000FF, 0); 
	paletteSetColor(COL_MAGENTA, 0xFF00FF, 0); 
	paletteSetColor(COL_CYAN, 0x00FFFF, 0); 
	paletteSetColor(COL_WHITE, 0xFFFFFF, 0); 
	paletteSetColor(COL_BLACK, 0x000000, 0); 

	paletteSet();
}
