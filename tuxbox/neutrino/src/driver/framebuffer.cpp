#include "framebuffer.h"


CFrameBuffer::CFrameBuffer(const char *fb)
{
	iconBasePath = "";
	available=0;
	cmap.start=0;
	cmap.len=256;
	cmap.red=red;
	cmap.green=green;
	cmap.blue=blue;
	cmap.transp=trans;
	backgroundColor = 0;
	useBackgroundPaint = false;
	background = NULL;

	fd=open(fb, O_RDWR);
	if (fd<0)
	{
		perror(fb);
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
	lfb=(unsigned char*)mmap(0, available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
  
	if (!lfb)
	{
		perror("mmap");
		goto nolfb;
	}

	return;

	nolfb:
		printf("framebuffer not available.\n");
		lfb=0;
}


CFrameBuffer::~CFrameBuffer()
{
	if(background)
	{
		delete[] background;
	}

	/*
	if (available)
		ioctl(fd, FBIOPUT_VSCREENINFO, &oldscreen);
	if (lfb)
		munmap(lfb, available);
		*/
}

int CFrameBuffer::setMode(unsigned int nxRes, unsigned int nyRes, unsigned int nbpp)
{
	if (!available)
	{
		return -1;
	}

	screeninfo.xres_virtual=screeninfo.xres=nxRes;
	screeninfo.yres_virtual=screeninfo.yres=nyRes;
	screeninfo.bits_per_pixel=nbpp;

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo)<0)
	{
		perror("FBIOPUT_VSCREENINFO");
		return -1;
	}
	
	if ((screeninfo.xres!=nxRes) && (screeninfo.yres!=nyRes) && (screeninfo.bits_per_pixel!=nbpp))
	{
		printf("SetMode failed: wanted: %dx%dx%d, got %dx%dx%d\n",
		nxRes, nyRes, nbpp,
		screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);
		return -1;
	}

	xRes=screeninfo.xres;
	yRes=screeninfo.yres;
	bpp=screeninfo.bits_per_pixel;
	fb_fix_screeninfo fix;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
		return -1;
	}

	stride=fix.line_length;
	memset(lfb, 0, stride*yRes);
	return 0;
}


void CFrameBuffer::paletteFade(int i, __u32 rgb1, __u32 rgb2, int level)
{
  __u16 *r=cmap.red+i;
  __u16 *g=cmap.green+i;
  __u16 *b=cmap.blue+i;
  *r= ((rgb2&0xFF0000)>>16)*level;
  *g= ((rgb2&0x00FF00)>>8 )*level;
  *b= ((rgb2&0x0000FF)    )*level;
  *r+=((rgb1&0xFF0000)>>16)*(255-level);
  *g+=((rgb1&0x00FF00)>>8 )*(255-level);
  *b+=((rgb1&0x0000FF)    )*(255-level);
}

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

void CFrameBuffer::paletteSet()
{
	ioctl(fd, FBIOPUTCMAP, &cmap);
}


void CFrameBuffer::paintBoxRel(int x, int y, int dx, int dy, unsigned char col)
{
	unsigned char* pos = lfb + x + stride*y;
	for(int count=0;count<dy;count++)
	{
		memset(pos, col, dx);
		pos += stride;
	}
}

void CFrameBuffer::paintBox(int xa, int ya, int xb, int yb, unsigned char col)
{
	unsigned char* pos = lfb + xa + stride*ya;
	int dx = xb-xa; 
	int dy = yb-ya;
	for(int count=0;count<dy;count++)
	{
		memset(pos, col, dx);
		pos += stride;
	}
}

void CFrameBuffer::paintVLine(int x, int ya, int yb, unsigned char col)
{
	unsigned char* pos = lfb + x + stride*ya;
	int dy = yb-ya;
	for(int count=0;count<dy;count++)
	{
		*pos = col;
		pos += stride;
	}
}

void CFrameBuffer::paintVLineRel(int x, int y, int dy, unsigned char col)
{
	unsigned char* pos = lfb + x + stride*y;
	for(int count=0;count<dy;count++)
	{
		*pos = col;
		pos += stride;
	}
}

void CFrameBuffer::paintHLine(int xa, int xb, int y, unsigned char col)
{
	unsigned char* pos = lfb + xa + stride*y;
	int dx = xb -xa;
	memset(pos, col, dx);
}

void CFrameBuffer::paintHLineRel(int x, int dx, int y, unsigned char col)
{
	unsigned char* pos = lfb + x + stride*y;
	memset(pos, col, dx);
}

void CFrameBuffer::setIconBasePath(string iconPath)
{
	iconBasePath = iconPath;
}

bool CFrameBuffer::paintIcon8(string filename, int x, int y, unsigned char offset)
{
	short width, height;
	unsigned char tr;

	int fd;
	filename = iconBasePath + filename;

	fd = open(filename.c_str(), O_RDONLY );
	
	if (fd==-1)
	{
		printf("error while loading icon: %s", filename.c_str() );
		return false;
	}

	read(fd, &width,  2 );
	read(fd, &height, 2 );
	read(fd, &tr, 1 );

	width= ((width & 0xff00) >> 8) | ((width & 0x00ff) << 8);
	height=((height & 0xff00) >> 8) | ((height & 0x00ff) << 8);

	unsigned char pixbuf[768];
	unsigned char *d = lfb + x +stride*y;
	unsigned char *d2;
	for (int count=0; count<height; count ++ )
	{
		read(fd, &pixbuf, width );
		unsigned char *pixpos = (unsigned char*) &pixbuf;
		d2 = d;
		for (int count2=0; count2<width; count2 ++ )
		{
			unsigned char color = *pixpos;
			*d2 = color + offset;
			d2++;
			pixpos++;
		}
		d += stride;
	}
	close(fd);
	return true;
}

bool CFrameBuffer::paintIcon(string filename, int x, int y, unsigned char offset)
{
	short width, height;
	unsigned char tr;

	int fd;
	filename = iconBasePath + filename;

	fd = open(filename.c_str(), O_RDONLY );
	
	if (fd==-1)
	{
		printf("error while loading icon: %s", filename.c_str() );
		return false;
	}

	read(fd, &width,  2 );
	read(fd, &height, 2 );
	read(fd, &tr, 1 );

	width= ((width & 0xff00) >> 8) | ((width & 0x00ff) << 8);
	height=((height & 0xff00) >> 8) | ((height & 0x00ff) << 8);

	unsigned char pixbuf[768];
	unsigned char *d = lfb + x +stride*y;
	unsigned char *d2;
	for (int count=0; count<height; count ++ )
	{
		read(fd, &pixbuf, width >> 1 );
		unsigned char *pixpos = (unsigned char*) &pixbuf;
		d2 = d;
		for (int count2=0; count2<width >> 1; count2 ++ )
		{
			unsigned char compressed = *pixpos;
			unsigned char pix1 = (compressed & 0xf0) >> 4;
			unsigned char pix2 = (compressed & 0x0f);

			if (pix1 != tr)
			{
				*d2 = pix1 + offset;
			}
			d2++;
			if (pix2 != tr)
			{
				*d2 = pix2 + offset;
			}
			d2++;
			pixpos++;
		}
		d += stride;
	}
	
	close(fd);
	return true;
}
void CFrameBuffer::loadPal(string filename, unsigned char offset, unsigned char endidx )
{
	int fd;
	struct rgbData rgbdata;
	filename = iconBasePath + filename;

	fd = open(filename.c_str(), O_RDONLY );
	
	if (fd==-1)
	{
		printf("error while loading palette: %s", filename.c_str() );
		return;
	}

	int pos = 0;
	int readb = read(fd, &rgbdata,  sizeof(rgbdata) );
	while(readb)
	{ 
		__u32 rgb = (rgbdata.r<<16) | (rgbdata.g<<8) | (rgbdata.b);
		int colpos = offset+pos;
		if( colpos>endidx)
			break;
		paletteSetColor(colpos, rgb ,0);
		readb = read(fd, &rgbdata,  sizeof(rgbdata) );
		pos++;
	}
	paletteSet();
	close(fd);
}

void CFrameBuffer::paintPixel(int x, int y, unsigned char col)
{
	unsigned char* pos = lfb + x + stride*y;
	*pos = col;
}

void CFrameBuffer::paintLine(int xa, int ya, int xb, int yb, unsigned char col)
{
  int dx = abs (xa - xb), dy = abs (ya - yb);
  int p = 2 * dy - dx;
  int twoDy = 2 * dy, twoDyDx = 2 * (dy - dx);
  int x, y, xEnd;

  /* Determine which point to use as start, which as end */
  if (xa > xb) {
    x = xb;
    y = yb;
    xEnd = xa;
  }
  else {
    x = xa;
    y = ya;
    xEnd = xb;
  }
  paintPixel (x, y, col);

  while (x < xEnd) {
    x++;
    if (p < 0)
      p += twoDy;
    else {
      y++;
      p += twoDyDx; 
    }
    paintPixel (x, y, col);
  }
}

void CFrameBuffer::setBackgroundColor(int color)
{
	backgroundColor = color;
}

bool CFrameBuffer::loadBackground(string filename, unsigned char col)
{
	if(background)
	{
		delete[] background;
	}
	
	short width, height;
	unsigned char tr;
	int fd;
	filename = iconBasePath + filename;

	fd = open(filename.c_str(), O_RDONLY );
	
	if (fd==-1)
	{
		printf("error while loading icon: %s", filename.c_str() );
		return false;
	}

	read(fd, &width,  2 );
	read(fd, &height, 2 );
	read(fd, &tr, 1 );

	width= ((width & 0xff00) >> 8) | ((width & 0x00ff) << 8);
	height=((height & 0xff00) >> 8) | ((height & 0x00ff) << 8);

	if((width!=720) || (height!=576))
	{
		printf("error while loading icon: %s - invalid resulution = %dx%d", filename.c_str(), width, height );
		return false;
	}

	background = new unsigned char[720*576];
	
	read(fd, background, 720*576);
	
	if(col!=0)//pic-offset
	{
		unsigned char *bpos = background;
		int pos = 720*576;
		while(pos>0)
		{
			*bpos += col;
			bpos += 1;
			pos--;
		}
	}

	close(fd);
	return false;
}

void CFrameBuffer::useBackground(bool ub)
{
	useBackgroundPaint = ub;
}

void CFrameBuffer::paintBackgroundBox(int xa, int ya, int xb, int yb)
{
	int dx = xb - xa;
	int dy = yb - ya;
	paintBackgroundBoxRel(xa, ya, dx, dy);
}

void CFrameBuffer::paintBackgroundBoxRel(int x, int y, int dx, int dy)
{
	if(!useBackgroundPaint)
	{
		paintBoxRel(x, y, dx, dy, backgroundColor);
	}
	else
	{
		unsigned char *fbpos = lfb + x + stride*y;
		unsigned char *bkpos = background + x + stride*y;
		for(int count=0;count<dy;count++)
		{
			memcpy(fbpos, bkpos, dx);
			fbpos += stride;
			bkpos += stride;
		}
	}
}
