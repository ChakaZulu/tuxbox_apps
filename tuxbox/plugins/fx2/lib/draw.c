/*
** initial coding by fx2
*/



#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stropts.h>
#include <malloc.h>
#include <linux/fb.h>
#include <sys/time.h>
#include <sys/mman.h>

#ifndef USEX

#ifdef __i386__
#define fbdevname	"/dev/fb0"
#else
#define fbdevname	"/dev/fb/0"
#endif

#ifndef abs
#define abs(a)	((a>0)?a:-(a))
#endif

#include <draw.h>
#include <rcinput.h>

static	int							fd = -1;
static	struct fb_var_screeninfo	screeninfo;
static	struct fb_var_screeninfo	oldscreen;
static	int							available = 0;
static	unsigned char				*lfb = 0;
static	int							stride;
static	int							bpp = 8;
static	struct fb_cmap				cmap;
static	unsigned short				red[ 256 ];
static	unsigned short				green[ 256 ];
static	unsigned short				blue[ 256 ];
static	unsigned short				trans[ 256 ];
static	int							lastcolor=0;
extern	int							actcode;

void	FBSetColor( int idx, uchar r, uchar g, uchar b )
{
	red[idx] = r<<8;
	green[idx] = g<<8;
	blue[idx] = b<<8;
	trans[idx] = 0;

	if ( idx > lastcolor )
		lastcolor=idx;
}

int	FBInitialize( int xRes, int yRes, int nbpp, int extfd )
{
	struct fb_fix_screeninfo	fix;

	if ( extfd != -1 )
	{
		fd = extfd;
	}
	else
	{
		fd = open( fbdevname, O_RDWR );
		if ( fd == -1 )
		{
			perror("failed - open " fbdevname);
			return(-1);
		}
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo)<0)
	{
		perror("failed - FBIOGET_VSCREENINFO");
		return(-1);
	}
	memcpy(&oldscreen,&screeninfo,sizeof(screeninfo));

	screeninfo.xres_virtual=screeninfo.xres=xRes;
	screeninfo.yres_virtual=screeninfo.yres=yRes;
	screeninfo.bits_per_pixel=nbpp;

	cmap.start=0;
	cmap.len=256;
	cmap.red=red;
	cmap.green=green;
	cmap.blue=blue;
	cmap.transp=trans;

	memset(red,100,sizeof(unsigned short)*256);
	memset(green,100,sizeof(unsigned short)*256);
	memset(blue,100,sizeof(unsigned short)*256);
	memset(trans,0xffff,sizeof(unsigned short)*256);

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo)<0)
		perror("FBSetMode");
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo)<0)
		perror("failed - FBIOGET_VSCREENINFO");

	FBSetColor( BLACK, 1, 1, 1 );
    FBSetColor( BNR0, 1, 1, 1 ); 
    FBSetColor( WHITE, 255, 255, 255 );

	if (ioctl(fd, FBIOPUTCMAP, &cmap )<0)
		perror("FBSetCMap");

	bpp = screeninfo.bits_per_pixel;

	if ( bpp != 8 )
		return(-1);

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("failed - FBIOGET_FSCREENINFO");
		return(-1);
	}

	available=fix.smem_len;
	stride = fix.line_length;
	lfb=(unsigned char*)mmap(0,available,PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

	if ( !lfb )
	{
		perror("failed - mmap");
		return(-1);
	}

	memset(lfb,BLACK,stride * screeninfo.yres);

	return 0;
}

void	FBSetupColors( void )
{
	if (ioctl(fd, FBIOPUTCMAP, &cmap )<0)
		perror("FBSetCMap");
}

void	FBClose( void )
{
	/* clear screen */
	memset(lfb,0,stride * screeninfo.yres);

	if (available)
	{
        ioctl(fd, FBIOPUT_VSCREENINFO, &oldscreen);
		if (lfb)
			munmap(lfb, available);
	}
}

void	FBPaintPixel( int x, int y, unsigned char farbe )
{
	*(lfb + stride*y + x) = farbe;
}

void	FBDrawLine( int xa, int ya, int xb, int yb, unsigned char farbe )
{
	int dx = abs (xa - xb);
	int	dy = abs (ya - yb);
	int	x;
	int	y;
	int	End;
	int	step;

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

		FBPaintPixel (x, y, farbe);

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
			FBPaintPixel (x, y, farbe);
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

		FBPaintPixel (x, y, farbe);

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
			FBPaintPixel (x, y, farbe);
		}
	}
}

void	FBDrawHLine( int x, int y, int dx, unsigned char farbe )
{
	memset(lfb+x+stride*y,farbe,dx);
}

void	FBDrawVLine( int x, int y, int dy, unsigned char farbe )
{
	unsigned char	*pos = lfb + x + stride*y;
	int				i;

	for( i=0; i<dy; i++, pos += stride )
		*pos = farbe;
}

void	FBFillRect( int x, int y, int dx, int dy, unsigned char farbe )
{
	unsigned char	*pos = lfb + x + stride*y;
	int				i;

	for( i=0; i<dy; i++, pos += stride )
		memset(pos,farbe,dx);
}

void	FBDrawRect( int x, int y, int dx, int dy, unsigned char farbe )
{
	FBDrawHLine( x, y, dx, farbe );
	FBDrawHLine( x, y+dy, dx, farbe );
	FBDrawVLine( x, y, dy, farbe );
	FBDrawVLine( x+dx, y, dy, farbe );
}

void	FBCopyImage( int x, int y, int dx, int dy, unsigned char *src )
{
	int		i;

	if ( !dx || !dy )
		return;
	for( i=0; i < dy; i++ )
		memcpy(lfb+(y+i)*stride+x,src+dx*i,dx);
}

void	FBCopyImageCol( int x, int y, int dx, int dy, unsigned char col,
				unsigned char *src )
{
	int				i;
	int				k;
	unsigned char	*to;

	if ( !dx || !dy )
		return;
	for( i=0; i < dy; i++ )
	{
		to=lfb+(y+i)*stride+x;
		for( k=0; k<dx; k++, to++,src++ )
			*to=(*src + col);
	}
}

void	FBOverlayImage( int x, int y, int dx, int dy,
				int relx,					/* left start in under */
				int rely,					/* top start in under */
				unsigned char c1,			/* color instead of white in src */
				unsigned char *src,			/* on top (transp) */
				unsigned char *under,		/* links drunter */
				unsigned char *right,		/* rechts daneben */
				unsigned char *bottom )		/* darunter */
{
	int				i;
	int				k;
	unsigned char	*p;

#define SWC(a)	((a)==WHITE?c1:(a))

	if ( !dx || !dy )
		return;
	for( i=0; i < dy; i++ )
	{
		p = lfb+(y+i)*stride+x;
		for( k=0; k < dx; k++, p++ )
		{
			if ( *(src+dx*i+k) != BLACK )
				*p = SWC(*(src+dx*i+k));
			else if ( relx )
			{
				if ( relx+k >= dx )		/* use rigth */
				{
					if ( right )
						*p = *(right+dx*i+relx+k-dx);
					else
						*p = SWC(*(src+dx*i+k));
				}
				else
				{
					if ( under )
						*p = *(under+dx*i+relx+k);
					else
						*p = SWC(*(src+dx*i+k));
				}
			}
			else
			{
				if ( rely+i >= dy )		/* use bottom */
				{
					if ( bottom )
						*p = *(bottom+dx*(i+rely-dy)+k);
					else
						*p = SWC(*(src+dx*i+k));
				}
				else
				{
					if ( under )
						*p = *(under+dx*(i+rely)+k);
					else
						*p = SWC(*(src+dx*i+k));
				}
			}
		}
	}
}

void	FBPrintScreen( void )
{
	FILE			*fp;
	unsigned char	*p = lfb;
	int				y;
	int				x=0;

#define H(x)	((x/26)+65)
#define L(x)	((x%26)+65)

	fp = fopen( "/var/tmp/screen.xpm", "w" );
	if ( !fp )
	{
		return;
	}
	fprintf(fp,"/* XPM */\n");
	fprintf(fp,"static char *screen[] = {\n");
	fprintf(fp,"\"  %d    %d   %d    %d\"",screeninfo.xres,screeninfo.yres,
			lastcolor+1,lastcolor<=100?1:2);
	for( x=0; x < lastcolor; x++ )
	{
		if ( lastcolor <= 100 )
			fprintf(fp,",\n\"%c",x+65);
		else
			fprintf(fp,",\n\"%c%c",H(x),L(x));

		fprintf(fp, " c #%02x%02x%02x\"",
			(unsigned char)(red[x]>>8),
			(unsigned char)(green[x]>>8),
			(unsigned char)(blue[x]>>8));
	}
	for( y=0; y < screeninfo.yres; y++ )
	{
		fprintf(fp,",\n\"");
		for( x=0; x < screeninfo.xres; x++, p++ )
		{
			if ( lastcolor <= 100 )
				fprintf(fp,"%c",(*p)+65);
			else
				fprintf(fp,"%c%c",H(*p),L(*p));
		}
		fprintf(fp,"\"");
		fflush(fp);
	}
	fprintf(fp," };\n");
	fflush(fp);

	fclose(fp);
}

void	FBBlink( int x, int y, int dx, int dy, int count )
{
	unsigned char	*back;
	int				i;
	struct timeval	tv;

/* copy out */
	back = malloc(dx*dy);
	for( i=0; i < dy; i++ )
		memcpy(back+dx*i,lfb+(y+i)*stride+x,dx);

	for( i=0; i < count; i++ )
	{
		tv.tv_usec = 300000;
		tv.tv_sec = 0;
		select(0,0,0,0,&tv);
		FBFillRect( x, y, dx, dy, BLACK );
		tv.tv_usec = 300000;
		tv.tv_sec = 0;
		select(0,0,0,0,&tv);
		FBCopyImage( x, y, dx, dy, back );
	}
	free( back );
}

void	FBMove( int x, int y, int x2, int y2, int dx, int dy )
{
	unsigned char	*back;
	int				i;

/* copy out */
	back = malloc(dx*dy);
	for( i=0; i < dy; i++ )
		memcpy(back+dx*i,lfb+(y+i)*stride+x,dx);

/* copy in */
	FBCopyImage( x2, y2, dx, dy, back );

	free( back );
}

void	FBPause( void )
{
	unsigned char	*back;
	int				dx = screeninfo.xres;
	int				dy = screeninfo.yres;
	int				x;
	int				y;
	int				sx;
	int				sy;
	int				i;
	struct timeval	tv;
static int			pos[64] =
{
0, 2, 11, 23, 5, 31, 33, 29, 14, 53, 59, 36, 35, 3, 49, 1, 
16, 19, 56, 34, 58, 32, 51, 12, 4, 52, 63, 7, 57, 50, 6, 24, 
43, 48, 39, 8, 20, 44, 27, 42, 10, 55, 61, 21, 17, 37, 47, 25, 
54, 28, 18, 46, 60, 9, 30, 38, 62, 26, 15, 13, 41, 22, 40, 45
};

	back = malloc( available );
	if ( back )		// dimm out
	{
		memcpy(back,lfb,available);

		for( i=0; i < 64; i++ )
		{
			sx = pos[i] >> 3;
			sy = pos[i] & 0x7;
			for( y=0; y < dy; y+=8 )
			{
				for( x=0; x < dx; x+=8 )
				{
					FBPaintPixel(x+sx,y+sy,0);
				}
			}
		}
	}

	actcode = 0xee;

	while( actcode == 0xee )
	{
		tv.tv_usec = 300000;
		tv.tv_sec = 0;
		select(0,0,0,0,&tv);
		RcGetActCode();
	}

	if ( back )		// dimm in
	{
		for( i=0; i < 64; i++ )
		{
			sx = pos[i] >> 3;
			sy = pos[i] & 0x7;
			for( y=0; y < dy; y+=8 )
			{
				for( x=0; x < dx; x+=8 )
				{
					*(lfb+(y+sy)*stride+x+sx) = *(back+(y+sy)*stride+x+sx);
				}
			}
		}

		free(back);
	}

	tv.tv_usec = 0;
	tv.tv_sec = 2;
	select(0,0,0,0,&tv);
}

#else	/* USEX */

#include <X11/Xlib.h>
#include <X11/X.h>

#include <draw.h>
#include <rcinput.h>

static	Display			*dpy = 0;
static	Window			window;
static	GC				gc;
static	unsigned long	colors[ 256 ];
static	int				planes=16;
static	int				xres;
static	int				yres;
extern	int				actcode;

void	FBSetColor( int idx, uchar r, uchar g, uchar b )
{
	switch( planes )
	{
	case 24 :
		colors[idx] = r<<16 | g<<8 | b;
		break;
	case 16 :
		colors[idx] = (r&0xf8)<<8 | (g&0xfc) << 3 | (b&0xf8)>>3;
		break;
	}
}

void	FBSetupColors( void )
{
}

int	FBInitialize( int xRes, int yRes, int bpp, int extfd )
{
	dpy = XOpenDisplay(0);
	if ( !dpy )
		return -1;

	xres = xRes;
	yres = yRes;

	window = XCreateSimpleWindow(dpy,RootWindow(dpy,0),
			100, 100, xRes, yRes, 0, 0, 0 );

	XMapWindow(dpy,window);

	planes=DisplayPlanes(dpy,0);

	gc = XCreateGC( dpy, window, 0, 0 );
	XSetFunction( dpy,gc,GXcopy);

	FBSetColor( BLACK, 1, 1, 1 );
    FBSetColor( BNR0, 1, 1, 1 ); 
    FBSetColor( WHITE, 255, 255, 255 );

	XFlush(dpy);
	return dpy ? 0 : -1;
}

void	FBClose( void )
{
	XCloseDisplay(dpy);
}

void	FBPaintPixel( int x, int y, unsigned char col )
{
	XSetForeground(dpy,gc,colors[col]);
	XDrawPoint( dpy, window, gc, x, y );
}

void	FBDrawLine( int xa, int ya, int xb, int yb, unsigned char col )
{
	XSetForeground(dpy,gc,colors[col]);
	XDrawLine( dpy, window, gc, xa, ya, xb, yb );
}

void	FBDrawHLine( int x, int y, int dx, unsigned char col )
{
	XSetForeground(dpy,gc,colors[col]);
	XDrawLine( dpy, window, gc, x, y, x+dx, y );
}

void	FBDrawVLine( int x, int y, int dy, unsigned char col )
{
	XSetForeground(dpy,gc,colors[col]);
	XDrawLine( dpy, window, gc, x, y, x, y+dy );
}

void	FBFillRect( int x, int y, int dx, int dy, unsigned char col )
{
	XSetForeground(dpy,gc,colors[col]);
	XFillRectangle( dpy, window, gc, x, y, dx, dy );
}

void	FBDrawRect( int x, int y, int dx, int dy, unsigned char col )
{
	XSetForeground(dpy,gc,colors[col]);
	XDrawRectangle( dpy, window, gc, x, y, dx, dy );
}

void	FBCopyImage( int x, int y, int dx, int dy, unsigned char *src )
{
	int	i;
	int	k;

	for( k=0; k < dy; k++ )
		for( i=0; i < dx; i++, src++ )
			FBPaintPixel(i+x,k+y,*src);
}

void	FBOverlayImage(int x, int y, int dx, int dy, int relx, int rely,
					unsigned char c1,
					unsigned char *src,
					unsigned char *under,
					unsigned char *right,
					unsigned char *bottom )
{
	int				i;
	int				k;

#define SWC(a)	((a)==WHITE?c1:(a))

	if ( !dx || !dy )
		return;
	for( i=0; i < dy; i++ )
	{
		for( k=0; k < dx; k++ )
		{
			if ( *(src+dx*i+k) != BLACK )
				FBPaintPixel(k+x,i+y,SWC(*(src+dx*i+k)));
			else if ( relx )
			{
				if ( relx+k >= dx )		/* use rigth */
				{
					if ( right )
						FBPaintPixel(k+x,i+y,*(right+dx*i+relx+k-dx));
					else
						FBPaintPixel(k+x,i+y,SWC(*(src+dx*i+k)));
				}
				else
				{
					if ( under )
						FBPaintPixel(k+x,i+y,*(under+dx*i+relx+k));
					else
						FBPaintPixel(k+x,i+y,SWC(*(src+dx*i+k)));
				}
			}
			else
			{
				if ( rely+i >= dy )		/* use bottom */
				{
					if ( bottom )
						FBPaintPixel(k+x,i+y,*(bottom+dx*(i+rely-dy)+k));
					else
						FBPaintPixel(k+x,i+y,SWC(*(src+dx*i+k)));
				}
				else
				{
					if ( under )
						FBPaintPixel(k+x,i+y,*(under+dx*(i+rely)+k));
					else
						FBPaintPixel(k+x,i+y,SWC(*(src+dx*i+k)));
				}
			}
		}
	}
}

void	FBCopyImageCol( int x, int y, int dx, int dy, unsigned char col,
					unsigned char *src )
{
	int	i;
	int	k;

	for( k=0; k < dy; k++ )
		for( i=0; i < dx; i++, src++ )
			FBPaintPixel(i+x,k+y,*src+col);
}

void	FBBlink( int x, int y, int dx, int dy, int count )
{
}

void	FBMove( int x, int y, int x2, int y2, int dx, int dy )
{
	XCopyArea( dpy, window, window, gc, x, y, dx, dy, x2, y2 );
}

void	FBPrintScreen( void )
{
}

void	FBFlushGrafic( void )
{
	XFlush(dpy);
}

void	FBPause( void )
{
	int				dx = xres;
	int				dy = yres;
	int				x;
	int				y;
	int				sx;
	int				sy;
	int				i;
	struct timeval	tv;
static int			pos[64] =
{
0, 2, 11, 23, 5, 31, 33, 29, 14, 53, 59, 36, 35, 3, 49, 1, 
16, 19, 56, 34, 58, 32, 51, 12, 4, 52, 63, 7, 57, 50, 6, 24, 
43, 48, 39, 8, 20, 44, 27, 42, 10, 55, 61, 21, 17, 37, 47, 25, 
54, 28, 18, 46, 60, 9, 30, 38, 62, 26, 15, 13, 41, 22, 40, 45
};

	for( i=0; i < 64; i++ )
	{
		sx = pos[i] >> 3;
		sy = pos[i] & 0x7;
		for( y=0; y < dy; y+=8 )
		{
			for( x=0; x < dx; x+=8 )
			{
				FBPaintPixel(x+sx,y+sy,0);
			}
		}
	}

printf("pause()\n");

	actcode = 0xee;

	while( actcode == 0xee )
	{
		tv.tv_usec = 300000;
		tv.tv_sec = 0;
		select(0,0,0,0,&tv);
		RcGetActCode();
	}
}

#endif
