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

#ifdef __i386__
#define fbdevname	"/dev/fb0"
#else
#define fbdevname	"/dev/fb/0"
#endif

#ifndef abs
#define abs(a)	((a>0)?a:-(a))
#endif

#include <draw.h>

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
