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

#define IDSTRING "fbv"
#define dbout(fmt, args...) printf( "FBD[%d] " fmt, (int)time(NULL), ## args)

//using namespace std;

class CPictureViewer
{
struct cformathandler 
{
    struct cformathandler *next;
    int (*get_size)(char *,int *,int*);
    int (*get_pic)(char *,unsigned char *,int,int);
    int (*id_pic)(char *);
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
	bool ShowImage(std::string filename);
	void SetScaling(ScalingMode s){m_scaling=s;}
	
private:
	CFormathandler *fh_root;
	ScalingMode m_scaling;
	int show_image(char *name);
	CFormathandler * fh_getsize(char *name,int *x,int *y);
	void init_handlers(void);
	void add_format(int (*picsize)(char *,int *,int*),int (*picread)(char *,unsigned char *,int,int), int (*id)(char*));

};


#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */

#endif
