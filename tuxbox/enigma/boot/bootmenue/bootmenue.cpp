/*
 * $Id: bootmenue.cpp,v 1.12 2005/10/01 21:58:54 digi_casi Exp $
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

#ifdef HAVE_DREAMBOX_HARDWARE
#include "bootmenue.h"

#define CONFIGFILE "/var/tuxbox/config/enigma/bootmenue.conf"
#define SCRIPTFILE "/tmp/bm.sh"

extern int fh_png_getsize(const char *, int *, int *, int, int);
extern int fh_png_load(const char *, unsigned char *, int, int);
extern int fh_png_id(const char *);

stmenu *stmenu::instance;
bool doexit = false;

stmenu::stmenu()
{
	instance = this;

	CONNECT(RcInput::getInstance()->selected, stmenu::rc_event);
	display = new fbClass();
	lcd = new CLCDDisplay();
	CONNECT(CTimer::getInstance()->selected, stmenu::timeout);
	if (loadconfig())
	{
		CTimer::getInstance()->start(timeoutValue);
		if (loadskin())
		{
			if (loadimagelist())
			{
				display->SetSAA(videoformat); //rgb = 0, fbas = 1, svideo = 2, component = 3;
				display->SetMode(720, 576, 16);
				showpic();

				display->RenderString(tmp_ver, ver_x + 25 + 10, ver_y, menu_xs - 25 - 10, CLCDDisplay::LEFT, ver_font, ver_r, ver_g, ver_b);

				display->Fill_buffer(menu_x - 5, menu_y - 5, menu_xs + 10, menu_ys + 10);

				drawmenu();
				mainloop();
			}
		}
	}
	
	delete display;
	delete lcd;
	imagelist.clear();
	printf("we are done.\n");
}

stmenu::~stmenu()
{
}

void stmenu::timeout()
{
	doexit = true;
}

void stmenu::rc_event(unsigned short key)
{
	CTimer::getInstance()->start(timeoutValue);

	switch (key)
	{
		case RC_EXIT: 
		case RC_OK: 
			doexit = true; 
			break;
		case RC_UP: 
			selentry--; 
			if (selentry < 0) 
				selentry = maxentry; 
			break;
		case RC_DOWN: 
			selentry++; 
			if (selentry > maxentry) 
			selentry = 0; 
			break;
		default: 
			break;
	}

	drawmenu();
}

void stmenu::drawmenu()
{
	int firstentry = 0;
	int lastentry = 0;

	if (selentry > 3)  firstentry = 4;
	if (selentry > 7)  firstentry = 8;
	if (selentry > 11) firstentry = 12;
	if (selentry > 15) firstentry = 16;
	if (selentry > 19) firstentry = 20;

	lastentry = firstentry + 3;
	if (lastentry > maxentry) 
		lastentry = maxentry;
		
	display->W_buffer(menu_x - 5, menu_y - 5, menu_xs + 10, menu_ys + 10);
	lcd->draw_fill_rect (0, 0, 120, 64, CLCDDisplay::PIXEL_OFF);
	
	int a = 0;
	int h = (menu_ys / 4) - 2;

	for (int i = firstentry; i <= lastentry; i++)
	{
		lcd->RenderString(imagelist[i].name, 0, (a * 15) + 3, 120, CLCDDisplay::CENTER, 20, CLCDDisplay::PIXEL_ON);
		int a1 = menu_y + (a * h);
		if (i == selentry)
		{
			lcd->draw_fill_rect (0, (a * 15), 120, (a * 15) + 15, CLCDDisplay::PIXEL_INV);
			//display->FillRect(menu_x + 5, a1, menu_xs + 25 + 5, h, sel_r, sel_g, sel_b);
			display->RenderCircle(menu_x + 10, a1 + 2 * (h / 3) - 2, sel_r, sel_g, sel_b);
		}
								//25 ist platz fuer Kreis
		display->RenderString(imagelist[i].name, menu_x + 25 + 10, a1 + h, menu_xs - 25 - 10, CLCDDisplay::LEFT, h, str_r, str_g, str_b);
		a++;
	}
	lcd->update();
}

void stmenu::mainloop()
{
	while (!doexit)
		usleep(50000);

	//clear menu
	lcd->draw_fill_rect (0, 0, 120, 64, CLCDDisplay::PIXEL_OFF);
	lcd->update();
	display->SetMode(720, 576, 8);

	newscript(imagelist[selentry].location);
	goscript(imagelist[selentry].location);
	
	saveconfig();
}

bool stmenu::loadskin()
{
	bool showTitle = false;
	if (strlen(skin_path) > 0 && strlen(skin_name) > 0)
	{
		std::string tmp = std::string(skin_path) + "/" + std::string(skin_name);

		if (FILE *in = fopen(tmp.c_str(), "rt"))
		{
			char line[256];
			while (fgets(line, 256, in))
			{
				if (!strncmp(line, "first-line", 10))
				{
					sscanf(line, "first-line=%d,%d,%d,%d,%d,%d", &ver_x, &ver_y, &ver_font, &ver_r, &ver_g, &ver_b);
					showTitle = true;
				}
				else 
				if (!strncmp(line, "menu-size", 9))
					sscanf(line, "menu-size=%d,%d,%d,%d", &menu_x, &menu_y, &menu_xs, &menu_ys);
				else 
				if (!strncmp(line, "string-color", 12))
					sscanf(line, "string-color=%d,%d,%d", &str_r, &str_g, &str_b);
				else 
				if (!strncmp(line, "select-color", 12))
					sscanf(line, "select-color=%d,%d,%d", &sel_r, &sel_g, &sel_b);
			}
			fclose(in);
			if (!showTitle)
				tmp_ver = "";
			return true;
		}
	}
	return false;
}

void stmenu::showpic()
{
	if (strlen(skin_path) > 0 && strlen(skin_name) > 0)
	{
		char pic[1024];
		strcpy(pic, skin_path); strcat(pic, "/"); strcat(pic, skin_name);
		if (strlen(pic) > 4) 
			pic[strlen(pic) - 4] = 0;
		strcat(pic, "png");

		if (fh_png_id(pic) == 1)
		{
			int x, y;
			fh_png_getsize(pic, &x, &y, INT_MAX, INT_MAX);
			unsigned char *buffer = (unsigned char *)malloc(x * y * 3);

			if (fh_png_load(pic,buffer,x,y) == 0)
				display->fb_display(buffer, NULL, x, y, 0, 0, 0, 0);

			free(buffer);
		}
	}
}

bool stmenu::loadconfig()
{
	timeoutValue = 10;
	videoformat = 1;
	selentry_st[0] = '\0';
	strcpy(skin_path, "/share/tuxbox/enigma/boot");
	strcpy(skin_name, "blank.skin");
	strcpy(mpoint, "/var/mnt/usb");
	inetd = 0;
	if (FILE *in = fopen(CONFIGFILE, "rt"))
	{
		printf("[STARTMENU] config loaded\n");
		char line[256];
		while(fgets(line, 256, in))
		{
			if (line[strlen(line) - 2] == '\r') 
				line[strlen(line) - 2] = 0; 
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = 0;
			if (!strncmp(line, "timeout", 7))
				timeoutValue = atoi(line + 8);
			else 
			if (!strncmp(line, "videoformat", 11))
				videoformat = atoi(line + 12);
			else 
			if (!strncmp(line, "selentry", 8))
				sscanf(line, "selentry=%s", selentry_st);
			else 
			if (!strncmp(line, "skin-path", 9))
				sscanf(line, "skin-path=%s", skin_path);
			else 
			if (!strncmp(line, "skin-name", 9))
				sscanf(line, "skin-name=%s", skin_name);
			else 
			if (!strncmp(line, "mountpoint", 10))
				sscanf(line, "mountpoint=%s", mpoint);
			else 
			if (!strncmp(line, "kill_inetd", 10))	
				inetd = atoi(line + 11);
		}
		fclose(in);

		tmp_ver = std::string("BootManager - ") + std::string(VERSION);
	}
	else
		printf("[STARTMENU] <%s not found>, using defaults...\n", CONFIGFILE);

	return true;
}

void stmenu::saveconfig()
{
	if (FILE *f = fopen(CONFIGFILE, "w"))
	{
		fprintf(f, "#BootManager-Config\n");
		fprintf(f, "mountpoint=%s\n", mpoint);
		fprintf(f, "selentry=%s\n", imagelist[selentry].location.c_str());
		fprintf(f, "kill_inetd=%d\n", inetd);
		fprintf(f, "timeout=%d\n", timeoutValue);
		fprintf(f, "videoformat=%d\n", videoformat);
		fprintf(f, "skin-path=%s\n", skin_path);
		fprintf(f, "skin-name=%s\n", skin_name);
		fclose(f);
	}
}

bool stmenu::loadimagelist()
{
	struct stat s;
	image a;
	
	std::string dir[2];
	dir[0] = std::string(mpoint) + "/image/";
	dir[1] = std::string(mpoint) + "/fwpro/";
	
	imagelist.clear();
	a.name = "Flash-Image";
	a.location  = "";
	imagelist.push_back(a);

	selentry = 0;
	int curentry = 0;
	for (int i = 0; i < 2; i++)
	{
		DIR *d = opendir(dir[i].c_str());
		if (d)
		{
			while (struct dirent *e = readdir(d))
			{
				if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
				{
					std::string name = dir[i] + e->d_name;
					stat(name.c_str(), &s);
					if (S_ISDIR(s.st_mode))
					{
						std::string tmp = e->d_name;
						a.location = name;
						a.name = e->d_name;
						tmp = name + "/imagename";
						if (FILE *in = fopen(tmp.c_str(), "rt"))
						{
							char line[256];
							line[0] = '\0';
							fgets(line, 256, in);
							fclose(in);
							if (strlen(line) > 0)
							{
								line[strlen(line) - 1] = '\0';
								a.name = std::string(line);
							}
						}
	
						imagelist.push_back(a);

						curentry++;
						if (strcmp(selentry_st, name.c_str()) == 0)
							selentry = curentry;
					}
				}
			}
			closedir(d);
		}
	}

	maxentry = imagelist.size() - 1;
	return maxentry > 0;
}

void stmenu::newscript(std::string image)
{
	if (FILE *f = fopen(SCRIPTFILE, "w"))
	{
		fprintf(f, "#!/bin/sh\n\n");
		if (image != "")
		{
			ProcUtils::killProcess("smbd");
			ProcUtils::killProcess("nmbd");
			if (inetd == 1) 
				ProcUtils::killProcess("inetd");
				
			fprintf(f, "killall -9 rcS\n");
			fprintf(f, "killall -9 init\n");
			if (strcmp(mpoint, "/hdd"))
				fprintf(f, "umount /hdd\n");
			fprintf(f, "rm %s\n",SCRIPTFILE);
			fprintf(f, "chroot %s ../go\n", image.c_str());
		}
		else
			fprintf(f, "echo booting flash image...\n");
			
		fclose(f);
		system("chmod 755 "SCRIPTFILE);
	}
}

void stmenu::goscript(std::string image)
{
	std::string go = image + "/go";
	if (FILE *f = fopen(go.c_str(), "w"))
	{
		fprintf(f, "#!/bin/sh\n\n");		
		fprintf(f, "mount -t devfs dev /dev\n");
		fprintf(f, "/etc/init.d/rcS&\n");
		fclose(f);
		system(std::string("chmod 755 " + go).c_str());
	}
}

int main(int argc, char **argv)
{
	stmenu::getInstance();
	return 0;
}
#else
int main(int argc, char **argv)
{
	return 0;
}
#endif
