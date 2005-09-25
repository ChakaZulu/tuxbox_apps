/*
 * $Id: bootmenue.h,v 1.5 2005/09/25 15:06:21 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <dirent.h>

#include "my_fb.h"
#include "my_lcd.h"
#include "my_rc.h"
#include "my_timer.h"
#include "processutils.h"

#define VERSION "0.0.2"

class image
{
 public:
	std::string name;
	std::string location;
};

class stmenu: public Object
{
	static stmenu *instance;
	fbClass *display;
	CLCDDisplay *lcd;

	std::vector<image> imagelist;

	int ver_x, ver_y, ver_font, ver_r, ver_g, ver_b;
	int menu_x, menu_y, menu_xs, menu_ys;
	int str_r, str_g, str_b;
	int sel_r, sel_g, sel_b;

	int timeoutValue, videoformat, selentry, maxentry;
	char version[256], selentry_st[256], skin_path[256], skin_name[256];
	std::string tmp_ver;

	char mpoint[256];
	int inetd;

	void rc_event(unsigned short key);
	void mainloop();
	bool loadconfig();
	void saveconfig();
	bool loadskin();
	bool loadimagelist();
	void timeout();

	void newscript(std::string image);
	void goscript(std::string image);

	void drawmenu();
	void showpic();

 public:
	static stmenu *getInstance() {return (instance) ? instance : instance = new stmenu();}
	stmenu();
	~stmenu();
 private:
 	static void timeout(int);
};
