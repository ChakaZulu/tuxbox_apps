/*
	pictureviewer  -   DBoxII-Project

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

#ifndef __pictureviewer__
#define __pictureviewer__
#include <stdio.h>
#include <jpeglib.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

class CPictureViewer
{
struct cformathandler 
{
    struct cformathandler *next;
    int (*get_size)(const char *,int *,int*,int);
    int (*get_pic)(const char *,unsigned char *,int,int);
    int (*id_pic)(const char *);
};
typedef  struct cformathandler CFormathandler;

public:
	enum ScalingMode
	{
		NONE=0,
		SIMPLE=1,
		COLOR=2
	};
	CPictureViewer();
	~CPictureViewer(){};
	bool ShowImage(std::string filename, int startx, int endx, int starty, int endy);
	bool DecodeImage(std::string name, int startx, int endx, int starty, int endy, bool showBusySign=false);
   bool DisplayImage();
	void SetScaling(ScalingMode s){m_scaling=s;}
	void SetAspectRatio(float aspect_ratio) {m_aspect=aspect_ratio;}
	void showBusy(int sx, int sy, int width, char r, char g, char b);
	void hideBusy();
	
private:
	CFormathandler *fh_root;
	ScalingMode m_scaling;
	float m_aspect;
   std::string m_Pic_Name;
   unsigned char* m_Pic_Buffer;
   int m_Pic_X;
   int m_Pic_Y;
   int m_Pic_XPos;
   int m_Pic_YPos;
   int m_Pic_XPan;
   int m_Pic_YPan;
	
	unsigned char* m_busy_buffer;
	int m_busy_x;
	int m_busy_y;
	int m_busy_width;
	int m_busy_cpp;
	
	CFormathandler * fh_getsize(const char *name,int *x,int *y, int width_wanted);
	void init_handlers(void);
	void add_format(int (*picsize)(const char *,int *,int*,int),int (*picread)(const char *,unsigned char *,int,int), int (*id)(const char*));

};


#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */

#endif
