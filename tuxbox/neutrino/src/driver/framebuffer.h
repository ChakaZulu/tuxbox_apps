/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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


#ifndef __framebuffer__
#define __framebuffer__


#include <linux/fb.h>
#include <linux/vt.h>

#include <stdint.h>
#include <string>

typedef struct fb_var_screeninfo t_fb_var_screeninfo;

/** Ausführung als Singleton */
class CFrameBuffer
{
	private:

		CFrameBuffer();

		struct rgbData
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
		} __attribute__ ((packed));

		struct rawHeader
		{
			uint8_t width_lo;
			uint8_t width_hi;
			uint8_t height_lo;
			uint8_t height_hi;
			uint8_t transp;
		} __attribute__ ((packed));

		std::string     iconBasePath;

		int             fd, tty;
		unsigned char * lfb;
		int		available;
		uint8_t *       background;
		uint8_t *       backupBackground;
		int             backgroundColor;
		std::string     backgroundFilename;
		bool            useBackgroundPaint;
		unsigned int	xRes, yRes, stride, bpp;
		t_fb_var_screeninfo screeninfo, oldscreen;
		fb_cmap cmap;
		__u16 red[256], green[256], blue[256], trans[256];

		void paletteFade(int i, __u32 rgb1, __u32 rgb2, int level);

		int 	kd_mode;
		struct	vt_mode vt_mode;
		bool	active;
		static	void switch_signal (int);

	public:

		~CFrameBuffer();

		static CFrameBuffer* getInstance();

		void init(const char * const fbDevice = "/dev/fb/0");
		int setMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);


		int getFileHandle(); //only used for plugins (games) !!
		t_fb_var_screeninfo *getScreenInfo();

		unsigned char* getFrameBufferPointer(); //pointer to framebuffer
		unsigned int getStride(); //stride (anzahl bytes die eine Zeile im Framebuffer belegt)
		bool getActive(); //is framebuffer active

		void setTransparency( int tr = 0 );

		//Palette stuff
		void setAlphaFade(int in, int num, int tr);
		void paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
		void paletteSetColor(int i, __u32 rgb, int tr);
		void paletteSet(struct fb_cmap *map = NULL);

		//paint functions
		void paintPixel(int x, int y, unsigned char col);

		void paintBoxRel(const int x, const int y, const int dx, const int dy, const unsigned char col);
		inline void paintBox(int xa, int ya, int xb, int yb, unsigned char col) { paintBoxRel(xa, ya, xb - xa, yb - ya, col); }

		void paintLine(int xa, int ya, int xb, int yb, unsigned char col);

		void paintVLine(int x, int ya, int yb, unsigned char col);
		void paintVLineRel(int x, int y, int dy, unsigned char col);

		void paintHLine(int xa, int xb, int y, unsigned char col);
		void paintHLineRel(int x, int dx, int y, unsigned char col);


		void setIconBasePath(const std::string & iconPath);

		bool paintIcon (const char * const filename, const int x, const int y, const unsigned char offset = 1);
		bool paintIcon (const std::string & filename, const int x, const int y, const unsigned char offset = 1);
		bool paintIcon8(const std::string & filename, const int x, const int y, const unsigned char offset = 0);
		void loadPal   (const std::string & filename, const unsigned char offset = 0, const unsigned char endidx = 255);

		bool loadPicture2Mem        (const std::string & filename, uint8_t * const memp);
		bool loadPicture2FrameBuffer(const std::string & filename);
		bool loadPictureToMem       (const std::string & filename, const uint16_t width, const uint16_t height, const uint16_t stride, uint8_t * const memp);
		bool savePictureFromMem     (const std::string & filename, const uint8_t * const memp);

		int getBackgroundColor() { return backgroundColor;}
		void setBackgroundColor(int color);
		bool loadBackground(const std::string & filename, const unsigned char col = 0);
		void useBackground(bool);
		bool getuseBackground(void);

		void saveBackgroundImage(void);  // <- implies useBackground(false);
		void restoreBackgroundImage(void);

		void paintBackgroundBoxRel(int x, int y, int dx, int dy);
		inline void paintBackgroundBox(int xa, int ya, int xb, int yb) { paintBackgroundBoxRel(xa, ya, xb - xa, yb - ya); }

		void paintBackground();

		void SaveScreen(int x, int y, int dx, int dy, unsigned char* memp);
		void RestoreScreen(int x, int y, int dx, int dy, unsigned char* memp);

		void ClearFrameBuffer();

};


#endif
