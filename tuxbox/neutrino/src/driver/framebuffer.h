#ifndef __framebuffer__
#define __framebuffer__

#include <linux/fb.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>

#include <string>

using namespace std;


class CFrameBuffer
{
	private:
	
		struct rgbData
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
		};

		string iconBasePath;

		int				fd;
		int				available;
		unsigned char	*background;
		int				backgroundColor;
		bool			useBackgroundPaint;
		unsigned int xRes, yRes, stride, bpp;
		struct fb_var_screeninfo screeninfo, oldscreen;
		fb_cmap cmap;
		__u16 red[256], green[256], blue[256], trans[256];

		void paletteFade(int i, __u32 rgb1, __u32 rgb2, int level);

	public:
		//pointer to framebuffer
		unsigned char *lfb;
		unsigned int Stride() { return stride; }
  
		CFrameBuffer(const char *fb="/dev/fb/0");
		~CFrameBuffer();

		int setMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);


		//Palette stuff
		void paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
		void paletteSetColor(int i, __u32 rgb, int tr);
		void paletteSet();

		//paint functions
		void paintPixel(int x, int y, unsigned char col);
		
		void paintBox(int xa, int ya, int xb, int yb, unsigned char col);
		void paintBoxRel(int x, int y, int dx, int dy, unsigned char col);

		void paintLine(int xa, int ya, int xb, int yb, unsigned char col);

		void paintVLine(int x, int ya, int yb, unsigned char col);
		void paintVLineRel(int x, int y, int dy, unsigned char col);

		void paintHLine(int xa, int xb, int y, unsigned char col);
		void paintHLineRel(int x, int dx, int y, unsigned char col);


		bool paintIcon(string filename, int x, int y, unsigned char offset=0);
		bool paintIcon8(string filename, int x, int y, unsigned char offset=0);
		void loadPal(string filename, unsigned char offset=0, unsigned char endidx=255 );
		void setIconBasePath(string);


		void setBackgroundColor(int color);
		bool loadBackground(string filename, unsigned char col = 0);
		void useBackground(bool);

		void paintBackgroundBox(int xa, int ya, int xb, int yb);
		void paintBackgroundBoxRel(int x, int y, int dx, int dy);
};



#endif


