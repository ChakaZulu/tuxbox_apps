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

#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define IDSTRING "fbv"

using namespace std;


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

	CFormathandler *fh_root;


	int show_image(char *name);
	CFormathandler * fh_getsize(char *name,int *x,int *y);
	void init_handlers(void);
	void add_format(int (*picsize)(char *,int *,int*),int (*picread)(char *,unsigned char *,int,int), int (*id)(char*));

public:
	CPictureViewer(){fh_root=NULL;};
	~CPictureViewer(){};
	bool ShowImage(string filename);
};


#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */

#endif
