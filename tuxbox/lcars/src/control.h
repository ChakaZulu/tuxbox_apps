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

#include <vector>
#include <map>

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

	int runCommand(std::string command, bool value = true);

	// Modes
	typedef std::vector<std::string> commandlist;
	typedef std::map<int, commandlist> keylist;
	
	struct mode
	{
		int number;
		std::string name;
		keylist keys;
	};
	
	void loadModes();

	// Menue
	struct menu_entry
	{
		int type;
		std::string description;
		commandlist switches;
		commandlist value_commands;
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
	
	void loadMenus();
	void openMenu(int menuNumber);
	void getMenu(int menuNumber);
	void closeMenu();
	

public:
	control (osd *o, rc *r, hardware *h, settings *s, scan *s1, channels *c, eit *e, cam *c1, zap *z, tuner *t);


};

#endif
