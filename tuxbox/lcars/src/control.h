#ifndef CONTROL_H
#define CONTROL_H

#include "osd.h"
#include "rc.h"
#include "hardware.h"
#include "settings.h"
#include "scan.h"
#include "channels.h"
#include "eit.h"
#include "cam.h"
#include "zap.h"
#include "tuner.h"
#include "update.h"
#include "timer.h"
#include "plugins.h"
#include "checker.h"
#include "fbClass.h"
#include "variables.h"
#include "commandcoding.h"
#include "ir.h"
#include "pig.h"
#include "teletext.h"
#include "sdt.h"
#include "lcd.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>

#include <vector>
#include <map>

// Warum zur H÷lle schmeisst mir gcc da nen Fehler, wenn ich die struct
// in die Klasse nehme?! Gut, machen wir's halt global :(
struct command_class
{
	int if_type;
	std::string if_var;
	std::string if_value;

	int cmd_class;
	int command;
	std::vector<std::string> args;
	std::vector<int> var;
};

struct Service
{
	int TS;
	int ONID;
	int SID;
};

class control
{
	osd *osd_obj;
	rc *rc_obj;
	hardware *hardware_obj;
	settings *settings_obj;
	scan *scan_obj;
	channels *channels_obj;
	eit *eit_obj;
	cam *cam_obj;
	zap *zap_obj;
	tuner *tuner_obj;
	update *update_obj;
	timer *timer_obj;
	plugins *plugins_obj;
	checker *checker_obj;
	fbClass *fb_obj;
	ir *ir_obj;
	pig *pig_obj;
	teletext *teletext_obj;
	sdt *sdt_obj;
	lcd *lcd_obj;

	// Modes
	typedef std::vector<command_class> commandlist;
	typedef std::vector<std::string> string_commandlist;
	typedef std::map<int, commandlist> keylist;

	struct mode
	{
		int index;
		std::string title;
		commandlist init_commands;
		keylist keys;
	};

	int selected_channel;
	Service last_read;

	int current_mode;
	bool leave_mode;
	bool quit_modes;
	void loadModes();
	void runMode(int modeNumber);
	void runMode();
	std::map<int, mode> modes;

	// Subs
	std::map<std::string, commandlist> subs;
	void loadSubs();


	// Helps
	void dumpchannel(int channelnr);

	// Menue
	struct menu_entry
	{
		int type;
		std::string description;
		string_commandlist switches;
		string_commandlist value_commands;
		commandlist action_commands;
	};
	struct menu
	{
		std::string title;
		int index;
		commandlist init_commands;
		std::map<int, menu_entry> entries;
		std::vector<int> sort;
	};
	std::map<int, menu> menus;

	// Menues laden
	void loadMenus();

	// Menue oeffnen und starten
	void openMenu(int menuNumber);

	// Aktuelle Menuedaten laden
	void getMenu(int menuNumber);

	// Menue schliessen
	void closeMenu();


	// Aktuelles Kommando sofort ausfuehren
	bool checkSetting(std::string var);

	bool lastcheck;

	pthread_t thread;
	static void *startlistening(void *object);
public:
	variables *vars;
	void run();
	void startThread();
	control (osd *o, rc *r, hardware *h, settings *s, scan *s1, channels *c, eit *e, cam *c1, zap *z, tuner *t, update *u, timer *t1, plugins *p, checker *c2, fbClass *f, variables *v, ir *i, pig *p1, teletext *t2, sdt *s2, lcd *l);

	int runCommand(command_class command, bool value = true);
	command_class parseCommand(std::string cmd);

	void runSub(std::string);
	bool subAvailable(std::string);
};

#endif
