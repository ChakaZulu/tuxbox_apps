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
#include "processutils.h"
#include "my_timer.h"

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

	bool ver_use;
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
	bool loadskin();
	bool loadlist();
	void timeout();

	void newscript();

	void drawmenu();
	void showpic();

 public:
	static stmenu *getInstance() {return (instance) ? instance : instance = new stmenu();}
	stmenu();
	~stmenu();
 private:
 	static void timeout(int);
};
