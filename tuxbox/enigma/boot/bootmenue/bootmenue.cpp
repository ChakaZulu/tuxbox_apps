#include "bootmenue.h"

#ifdef dreamflash
#define CONFIGFILE	"/var/tuxbox/config/df.conf"
#define SCRIPTFILE	"/tmp/dfsh"
#else
#define CONFIGFILE	"/etc/bootmenue.conf"
#define SCRIPTFILE	"/usr/bin/startmenu.sh"
#endif

extern int fh_png_getsize(const char *, int *, int *, int, int);
extern int fh_png_load(const char *, unsigned char *, int, int);
extern int fh_png_id(const char *);

stmenu *stmenu::instance;
bool doexit = false;

stmenu::stmenu ()
{
	instance=this;
	ver_use=false;

	CONNECT(RcInput::getInstance()->selected, stmenu::rc_event);
	display = new fbClass();
	lcd = new CLCDDisplay();

	CONNECT(CTimer::getInstance()->selected, stmenu::timeout);

	if(loadconfig())
	{
		CTimer::getInstance()->start(timeoutValue);//timer starten
		loadskin();
		loadliste();
		display->SetSAA(videoformat);//rgb=0,fbas=1,svideo=2,component=3;
		display->SetMode( 720, 576, 16 );
		show_pic();

		if(ver_use)
			display->RenderString(tmp_ver, ver_x, ver_y, 400, 0, ver_font, ver_r, ver_g, ver_b);

		display->Fill_buffer(menu_x-5, menu_y-5, menu_xs+10, menu_ys+10);//ringrum um 5 größer

		drawMenu();
		main_loop();
	}
}

stmenu::~stmenu()
{
	delete display;
	delete lcd;
	imagelist.clear();
}

void stmenu::timeout()
{
	doexit = true;
}

void stmenu::rc_event(unsigned short key)
{
	CTimer::getInstance()->start(timeoutValue);//erneut timer starten

	switch(key)
	{
		case RC_EXIT: case RC_OK: doexit = true; break;
		case RC_UP: selentry--; if(selentry < 0) selentry=maxentry; break;//up
		case RC_DOWN: selentry++; if(selentry > maxentry) selentry=0; break;//down
		default: break;
	}

	drawMenu();
}

void stmenu::drawMenu()
{
	int firstentry = 0;
	int lastentry = 0;
	//was anzeigen
	if(selentry < 4)  firstentry=0;
	if(selentry > 3)  firstentry=4;
	if(selentry > 7)  firstentry=8;
	if(selentry > 11) firstentry=12;
	if(selentry > 15) firstentry=16;
	if(selentry > 19) firstentry=20;

	lastentry = firstentry + 3;
	if(lastentry > maxentry) lastentry=maxentry;

	//clear
	display->W_buffer(menu_x-5, menu_y-5, menu_xs+10, menu_ys+10);//ringrum um 5 größer
	lcd->draw_fill_rect (0, 0, 120, 64, CLCDDisplay::PIXEL_OFF);
	//fill
	int a=0;
	int h = (menu_ys/4) -2;

	for (int i = firstentry; i < lastentry+1; i++)
	{
		lcd->RenderString(imagelist[i].NAME, 0, (a*15)+3, 120, CLCDDisplay::CENTER, 20, CLCDDisplay::PIXEL_ON);
		int a1 = menu_y + (a*h);
		if(i == selentry)
		{
			lcd->draw_fill_rect (0, (a*15), 120, (a*15)+15, CLCDDisplay::PIXEL_INV);
			//display->FillRect( menu_x, a1, menu_xs, h, sel_r, sel_g, sel_b);
			display->RenderCircle( menu_x, a1 + 2*(h/3),sel_r, sel_g, sel_b);
		}
							//25 ist platz fuer Kreis
		display->RenderString(imagelist[i].NAME, menu_x + 25, a1+h, menu_xs - 25, CLCDDisplay::LEFT, h, str_r, str_g, str_b);
		a++;
	}
	lcd->update();
}

bool stmenu::main_loop()
{
	while (!doexit)
		usleep(50000);

	//clear menu
	lcd->draw_fill_rect (0, 0, 120, 64, CLCDDisplay::PIXEL_OFF);
	lcd->update();
	display->SetMode( 720, 576, 8 );

	//save config
	if(new_script())
	{
		if(FILE *f=fopen(CONFIGFILE, "w"))
		{
#ifdef dreamflash
			fprintf(f,"#DreamFlash-Config\n\nversion=%s\nmountpoint=%s\n",version,mpoint);
			fprintf(f,"kill_inetd=%d\nselentry=%s\n", inetd, imagelist[selentry].NAME.c_str());
#else
			fprintf(f,"#Startmenu-Config\n\nversion=%s\n",version);
			fprintf(f,"selentry=%s\n", imagelist[selentry].LOC.c_str());
#endif
			fprintf(f,"timeout=%d\nvideocolor=%d\n", timeoutValue, videoformat);
			fprintf(f,"skin-path=%s\nskin-name=%s\n", skin_path, skin_name);
			fclose(f);
		}
	}

	return true;
}

bool stmenu::loadskin()
{
	if(strlen(skin_path)>0 && strlen(skin_name)>0)
	{
		std::string tmp=skin_path;
		tmp += "/"; tmp += skin_name;

		if(FILE *in=fopen(tmp.c_str(), "rt"))
		{
			printf("[STARTMENU] skin loaded\n");
			char line[256];
			while(fgets(line, 256, in))
			{
				if (!strncmp(line, "first-line", 10))
				{
					ver_use=true;
					sscanf(line, "first-line=%d,%d,%d,%d,%d,%d", &ver_x, &ver_y, &ver_font, &ver_r, &ver_g, &ver_b);
				}
				else if (!strncmp(line, "menu-size", 9))
					sscanf(line, "menu-size=%d,%d,%d,%d", &menu_x, &menu_y, &menu_xs, &menu_ys);
				else if (!strncmp(line, "string-color", 12))
					sscanf(line, "string-color=%d,%d,%d", &str_r, &str_g, &str_b);
				else if (!strncmp(line, "select-color", 12))
					sscanf(line, "select-color=%d,%d,%d", &sel_r, &sel_g, &sel_b);
			}
			fclose(in);
			return true;
		}
	}

	printf("[STARTMENU] <skin not loaded>\n");
	ver_use=true;
	ver_x=190; ver_y=80; ver_font=70; ver_r=50; ver_g=50; ver_b=255;
	menu_x=150; menu_y=150; menu_xs=400; menu_ys=250;
	str_r=128; str_g=128; str_b=255;
	sel_r=200; sel_g=200; sel_b=200;

	return false;

}

bool stmenu::show_pic()
{
	if(strlen(skin_path)>0 && strlen(skin_name)>0)
	{
		char pic[1024];
		strcpy(pic,skin_path); strcat(pic,"/"); strcat(pic,skin_name);
		if(strlen(pic)>4) pic[strlen(pic)-4]=0;
		strcat(pic,"png");

		if(fh_png_id(pic)==1)
		{
			int x,y;
			fh_png_getsize(pic,&x,&y,INT_MAX,INT_MAX);
			unsigned char *buffer=(unsigned char *) malloc(x*y*3);

			if(fh_png_load(pic,buffer,x,y)==0)
				display->fb_display(buffer, NULL, x,y,0,0,0,0);//die beiden letzten für die Pos

			free(buffer);

			printf("[STARTMENU] loaded picture \"%s\"\n",pic);
			return true;
		}
	}

	printf("[STARTMENU] <picture not loaded>\n");
	return false;
}

bool stmenu::loadconfig()//fertig
{
	if(FILE *in=fopen(CONFIGFILE, "rt"))
	{
		printf("[STARTMENU] config loaded\n");
		char line[256];
		while(fgets(line, 256, in))
		{
			if(line[strlen(line)-2]=='\r') line[strlen(line)-2]=0;
			else if(line[strlen(line)-1]=='\n')line[strlen(line)-1]=0;

			if (!strncmp(line, "version", 7)) sscanf(line, "version=%s", version);
			else if (!strncmp(line, "timeout", 7))		timeoutValue = atoi(line+8);
			else if (!strncmp(line, "videocolor", 10))	videoformat = atoi(line+11);
			else if (!strncmp(line, "selentry", 8))		sscanf(line, "selentry=%s", selentry_st);
			else if (!strncmp(line, "skin-path", 9))	sscanf(line, "skin-path=%s", skin_path);
			else if (!strncmp(line, "skin-name", 9))	sscanf(line, "skin-name=%s", skin_name);
#ifdef dreamflash
			else if (!strncmp(line, "mountpoint", 10))	sscanf(line, "mountpoint=%s", mpoint);
			else if (!strncmp(line, "kill_inetd", 10))	inetd = atoi(line+11);
#endif
		}
		fclose(in);

		//obere Zeile
#ifdef dreamflash
		tmp_ver = "DreamFlash ";
#else
		tmp_ver = "Startmenu ";
#endif
		tmp_ver += version;

		//gibt es df.start
#ifdef dreamflash
		std::string df_start = mpoint; df_start += "/df.start";
		if(FILE *g=fopen(df_start.c_str(), "rt"))
		{
			fgets(line, 256, g);
			if(line[strlen(line)-1]=='\n')line[strlen(line)-1]=0;
			strcpy(selentry_st,line);
			fclose(g);
			unlink(df_start.c_str());
		}
#endif
	}
	else
	{
		printf("[STARTMENU] <%s not found>\n",CONFIGFILE);
		return false;
	}
	return true;
}

void stmenu::loadliste()//fertig
{
	struct stat s;
	imagelist.clear();

	IMAGE a;
#ifdef dreamflash
	a.NAME = "Flash-Image";
	a.LOC  = "";
	std::string dir = mpoint; dir += "/image/";
#else
	a.NAME = "Dream Multi Media";
	a.LOC  = "/usr/bin/dream_enigma.sh";
	std::string dir = "/usr/bin/";
#endif
	imagelist.push_back(a);

	selentry=0;
	int tmpcurr=0;
	DIR *d=opendir(dir.c_str());
	if(d)
	{
		while (struct dirent *e=readdir(d))
		{
			if (!(strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))) continue;

			std::string name = dir + e->d_name;
			std::string tmp = e->d_name;
			std::string tmp_file = name + "/go";

			stat(name.c_str(),&s);
#ifdef dreamflash
			if (S_ISDIR(s.st_mode))
			{
				std::string tmp_file = name + "/go";
				if(FILE *f=fopen(tmp_file.c_str(),"r"))
				{
					fclose(f);

					a.NAME = e->d_name;
					a.LOC  = name;
					imagelist.push_back(a);

					tmpcurr++;
					if (!strcmp(selentry_st,e->d_name)) selentry=tmpcurr;
				}
			}
#else
			if (S_ISDIR(s.st_mode))	continue;

			if(name.find("_enigma.sh")!= std::string::npos && tmp != "dream_enigma.sh")
			{
				int pos = tmp.find("_enigma.sh");
				tmp.erase(pos);

				a.NAME = tmp;
				a.LOC  = name;
				imagelist.push_back(a);

				tmpcurr++;
				if (name == selentry_st)	selentry=tmpcurr;
			}
#endif
		}
		closedir(d);
	}

	maxentry = imagelist.size()-1;
}

bool stmenu::new_script()//fertig
{
	if(FILE *f=fopen(SCRIPTFILE, "w"))
	{
#ifdef dreamflash
		ProcUtils::killProcess("smbd");
		ProcUtils::killProcess("nmbd");
		if (inetd==1) ProcUtils::killProcess("inetd");

		fprintf(f,"#!/bin/sh\n\n");
		fprintf(f,"killall -9 rcS\nkillall -9 init\n");
		if (strcmp(mpoint,"/hdd")) fprintf(f,"umount /hdd\n");
		fprintf(f, "rm %s\n",SCRIPTFILE);
		fprintf(f, "chroot %s ../go\n", imagelist[selentry].LOC.c_str());
#else
		fprintf(f,"#!/bin/sh\n\n.%s\n",imagelist[selentry].LOC.c_str());
#endif
		fclose(f);
		system("chmod 755 "SCRIPTFILE);
		return true;
	}

	return false;
}

int main(int argc, char **argv)
{
	if(FILE *f=fopen("/tmp/dfsh", "r"))
	{
		fclose(f);
		unlink("/tmp/dfsh");
	}

	stmenu::getInstance();
	return 0;
}

