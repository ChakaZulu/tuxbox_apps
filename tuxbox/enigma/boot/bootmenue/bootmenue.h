#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <dirent.h> //for Directory

#include "my_fb.h"
#include "my_lcd.h"
#include "my_rc.h"
#include "processutils.h"
#include "my_timer.h"

//#define dreamflash

class IMAGE
{
 public:
	std::string NAME;
	std::string LOC;
};

typedef std::vector<IMAGE> ImageListe;

class stmenu: public Object
{
	static stmenu *instance;
	fbClass *display;
	CLCDDisplay *lcd;

	ImageListe imagelist;

	bool ver_use;
	int ver_x, ver_y, ver_font, ver_r, ver_g, ver_b;
	int menu_x, menu_y, menu_xs, menu_ys;
	int str_r, str_g, str_b;
	int sel_r, sel_g, sel_b;

	int timeoutValue, videoformat, selentry, maxentry;
	char version[256], selentry_st[256], skin_path[256], skin_name[256];
	std::string tmp_ver;

#ifdef dreamflash
	char mpoint[256];
	int inetd;
#endif

	void rc_event(unsigned short key);
	bool main_loop();
	bool loadconfig();
	bool loadskin();
	void loadliste();
	void timeout();

	bool new_script();

	void drawMenu();
	bool show_pic();

 public:
	static stmenu *getInstance()
	{
		if (instance == NULL) instance = new stmenu();
		return instance;
	}
	stmenu();
	~stmenu();
 private:
 	static void timeout (int);
};
