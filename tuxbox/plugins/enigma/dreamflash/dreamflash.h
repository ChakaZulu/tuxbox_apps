#include <plugin.h>
#include <stdio.h>
#include <sys/mount.h> //for mount
#include <sys/stat.h> //for stat
#include <sys/vfs.h> //for statfs
#include <dirent.h> //for Directory
#include <fcntl.h>

#include <lib/gui/ewindow.h>
#include <lib/gui/emessage.h>
#include <lib/gui/elabel.h>
#include <lib/gui/textinput.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/listbox.h>
#include <lib/gui/enumber.h>
#include <lib/gdi/epng.h>//for loadPNG
#include <lib/gdi/gfbdc.h>//for loadPNG
#include <lib/system/econfig.h> //for video-format
#include <lib/gui/statusbar.h>

class setup_df: public eWindow
{
	eListBox<eListBoxEntryText> *mliste, *sliste;
	eStatusBar *status;
	eNumber *starttimer;
	eCheckbox *ch_inetd;
	eTextInputField *ed_skin_path;
	void okselected();
	void load_sliste();
	void new_init();
public:
	setup_df();
};

class info_df: public eWindow
{
	eLabel *x;
	eString ver;
	void Listeselchanged(eListBoxEntryText *item);
public:
	info_df();
};

class image_df: public eWindow
{
	eTextInputField *Iname;
	eButton *ok;
	eStatusBar *status;
	
	int was1, free_space;
	void oksel();
	bool image_install(const char *i_dir, const char *f_name);
	void Listeselchanged(eListBoxEntryText *item);
public:
	image_df(int was);
};

class df_main: public eWindow
{
	eListBox<eListBoxEntryText> *menuliste;
	void Listeselected(eListBoxEntryText *item);
public:
	df_main();
};

