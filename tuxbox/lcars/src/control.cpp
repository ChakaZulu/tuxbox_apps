#include "control.h"

control::control (osd *o, rc *r, hardware *h, settings *s, scan *s1, channels *c, eit *e, cam *c1, zap *z, tuner *t)
{
	osd_obj = o;
	rc_obj = r;
	hardware_obj = h;
	settings_obj = s;
	scan_obj = s1;
	channels_obj = c;
	eit_obj = e;
	cam_obj = c1;
	zap_obj = z;
	tuner_obj = t;


	loadMenus();
	openMenu(0);
}

int control::runCommand(std::string command, bool val = true)
{
	cout << "Executing: " << command << endl;
	std::istringstream iss(command);

	std::string cmd;
	std::string value;
	
	getline(iss, cmd, ' ');
	getline(iss, value, ' ');
	
	if (cmd == "HARDWARE")
	{
		if (value == "Set")
		{
			std::string value2;
			getline(iss, value2, ' ');
			if (value2 == "RGB")
			{
				hardware_obj->setOutputMode(OUTPUT_RGB);
				settings_obj->setOutputFormat(OUTPUT_RGB);
			}
			else if (value2 == "FBAS")
			{
				hardware_obj->setOutputMode(OUTPUT_FBAS);
				settings_obj->setOutputFormat(OUTPUT_FBAS);
			}
		}
		
	}
	else if (cmd == "SETTINGS")
	{
		if (value == "Set")
		{
			std::string value2;
			getline(iss, value2, ' ');
			if (value2 == "RCRepeat")
			{
				settings_obj->setRcRepeat(val);
			}
			else if (value2 == "SupportOldRC")
			{
				settings_obj->setSupportOldRc(val);
			}
			else if (value2 == "SwitchVCR")
			{
				settings_obj->setSwitchVCR(val);
			}
		}
	}
	else if (cmd == "CONTROL")
	{
		if (value == "Menu")
		{
			std::string value2;
			getline(iss, value2, ' ');
			if (value2 == "Mode")
			{
				std::string value3;
				getline(iss, value3, ' ');
				openMenu(atoi(value3.c_str()));
				return 1;
			}
		}
	}
	else if (cmd == "SCAN")
	{
		if (value == "Scan")
		{
			std::string value2;
			getline(iss, value2, ' ');
			if (value2 == "Normal")
			{
				*channels_obj = scan_obj->scanChannels();
				channels_obj->setStuff(eit_obj, cam_obj, hardware_obj, osd_obj, zap_obj, tuner_obj);
			}
			else if (value2 == "Update")
			{
				scan_obj->updateChannels(channels_obj);
			}
			else if (value2 == "Full")
			{
				*channels_obj = scan_obj->scanChannels(true);
				channels_obj->setStuff(eit_obj, cam_obj, hardware_obj, osd_obj, zap_obj, tuner_obj);
			}
		}
	}
	else if (cmd == "CHANNELS")
	{
		if (value == "Save")
		{
			std::string value2;
			getline(iss, value2, ' ');
			if (value2 == "DVB")
			{
				channels_obj->saveDVBChannels();
			}
		}
	}
	else
		cout << "Unknown command: " << command << endl;
	

	return 0;
}

void control::loadMenus()
{
	std::ifstream inFile;
	std::vector<std::string> line;
	
	inFile.open(CONFIGDIR "/lcars/menus.lcars");
	if (!inFile)
	{
		perror("menus.lcars");
	}
	
	std::string tmp_string;
	while(getline(inFile, tmp_string))
		line.insert(line.end(), tmp_string);
	inFile.close();
	
	int i = 0;
	while (i < line.size() - 1)
	{
		std::istringstream iss(line[i]);
		cout << "Endline: " << line[i] << endl;
		if (line[i++] != "------")
		{
			cout << "Error in menus.lcars: ------ missing in line " << (i + 1)<< endl;
		}

		menu menu;
		menu.entries.clear();
		while(line[i] == "+++")
		{
			i++;
			cout << i << " " << line[i] << endl;
			if (line[i] == "Title:")
			{
				i++;
				menu.title = line[i++];
				continue;
			}
			else if (line[i] == "Index:")
			{
				i++;
				menu.index = atoi(line[i++].c_str());
				continue;
			}
			else if (line[i] == "Init:")
			{
				i++;
				menu.init_commands.clear();
				while((i < (line.size() - 1)) && line[i] != "+++")
				{
					menu.init_commands.insert(menu.init_commands.end(), line[i]);
					i++;
				}
				if (line[i - 1] == "------")
					i--;
				continue;
			}
			else if (line[i] == "Entries:")
			{
				i++;
				int title_count = -200;
				menu.sort.clear();
				while((i < (line.size() - 1)) && line[i] != "+++")
				{
					std::string value;
					std::string description;
					std::istringstream iss(line[i]);

					getline(iss, value, ' ');
					getline(iss, description);

					int val = -1;
					int type = -1;
					commandlist tmp_switches;
					switch (value[0])
					{
					case '!':
						val = atoi(value.substr(1).c_str());
						type = 2;
						printf("%d\n", line[i+1][0]);
						while(line[++i][0] == 9)
						{
							tmp_switches.insert(tmp_switches.end(), line[i].substr(1, line[i].length() - 1));
							cout << "tmp_switches: " << line[i].substr(1, line[i].length() - 1) << endl;
						}
						i--;
						break;
					case 'x':
						val = atoi(value.substr(1).c_str());
						type = 1;
						break;
					case '*':
						val = title_count++;
						type = 3;
						break;
					default:
						val = atoi(value.c_str());
						type = 0;
						break;
					}
					

					if (menu.entries.count(val) == 0)
					{
						menu_entry tmp_entry;
						tmp_entry.description = description;
						tmp_entry.type = type;
						tmp_entry.switches = tmp_switches;
						cout << "Size: " << tmp_switches.size() << endl;
						menu.entries[val] = tmp_entry;
						menu.sort.insert(menu.sort.end(), val);
						cout << "Added " << description << " of type " << type << " as value " << val << endl;
					}
					else
					{
						std::map<int, menu_entry>::iterator it = menu.entries.find(val);
						it->second.description = description;
						it->second.type = type;
						it->second.switches = tmp_switches;
						cout << "Size: " << tmp_switches.size() << endl;
						menu.sort.insert(menu.sort.end(), val);
					}

					i++;
				}
				cout << menu.entries.size() << "Entries added" << endl;
				continue;
			}
			else if (line[i] == "Actions:")
			{
				i++;
				while(line[i] != "+++" && line[i] != "------" && i < line.size() )
				{
					int val = atoi(line[i].c_str());
					commandlist tmp_commands;
					while(line[++i][0] == 9)
					{
						tmp_commands.insert(tmp_commands.end(), line[i].substr(1, line[i].length() - 1));
						cout << val << " - " << line[i].substr(1, line[i].length() - 1) << endl;
					}
					if (menu.entries.count(val) == 0)
					{
						menu_entry tmp_entry;
						
						tmp_entry.action_commands = tmp_commands;
						menu.entries[val] = tmp_entry;
					}
					else
					{
						std::map<int, menu_entry>::iterator it = menu.entries.find(val);
						it->second.action_commands = tmp_commands;
					}
				}
				continue;
			}
			else if (line[i] == "Values:")
			{
				i++;
				while(line[i] != "+++" && line[i] != "------" && i < line.size() )
				{
					int val = atoi(line[i].c_str());
					commandlist tmp_commands;
					while(line[++i][0] == 9)
					{
						tmp_commands.insert(tmp_commands.end(), line[i].substr(1, line[i].length() - 1));
						cout << val << " - " << line[i].substr(1, line[i].length() - 1) << endl;
					}
					if (menu.entries.count(val) == 0)
					{
						menu_entry tmp_entry;
						
						tmp_entry.value_commands = tmp_commands;
						menu.entries[val] = tmp_entry;
					}
					else
					{
						std::map<int, menu_entry>::iterator it = menu.entries.find(val);
						it->second.value_commands = tmp_commands;
					}
				}
				continue;
			}
			else
			{
				cout << "Error in menus.lcars: unknown thing in line " << (i + 1)<< endl;
			}
			
			//i++;

		}
		cout << "Adding Index " << menu.index << endl;
		menus[menu.index] = menu;
		//getline(iss, cmd, '=');
		//getline(iss, parm, '=');
	}

	cout << "Read " << line.size() << " lines of Menu-Config" << endl;
}

void control::getMenu(int menuNumber)
{
	menu tmp_menu = menus[menuNumber];
	
	osd_obj->createMenu();
	osd_obj->setMenuTitle(tmp_menu.title);
	for (int i = 0; i < tmp_menu.sort.size(); i++)
	{
		std::map<int, menu_entry>::iterator it = tmp_menu.entries.find(tmp_menu.sort[i]);
		osd_obj->addMenuEntry(it->first, it->second.description, it->second.type);
		if (it->second.type == 2)
		{
			cout << "Size: " << it->second.switches.size() << endl;
			for (int j = 0; j < it->second.switches.size(); j++)
			{
				cout << "Value: " << it->second.switches[j] << endl;
				osd_obj->addSwitchParameter(i, it->second.switches[j]);
			}
			osd_obj->setSelected(i, 0);
		}
	}
}

void control::openMenu(int menuNumber)
{
	menu tmp_menu = menus[menuNumber];

	getMenu(menuNumber);
	
	osd_obj->addCommand("SHOW menu");
	osd_obj->addCommand("COMMAND menu select next");

	int key;
	do
	{
		int number = 0;
		key = rc_obj->read_from_rc();
		number = rc_obj->get_number();

		if (key == RC1_DOWN)
		{
			osd_obj->selectNextEntry();
		}
		else if (key == RC1_UP)
		{
			osd_obj->selectPrevEntry();
		}
		else if (key == RC1_OK)
		{
			number = osd_obj->menuSelectedIndex();
		}
		if (number != -1)
		{
			osd_obj->selectEntry(number);

			std::map<int, menu_entry>::iterator it = tmp_menu.entries.find(number);
			if (it->second.type == 2 || it->second.type == 1)
			{
				bool found = false;
				int i = 0;
				for (i = 0; i < tmp_menu.sort.size(); i++)
				{
					if (tmp_menu.sort[i] == number)
					{
						found = true;
						break;
					}
				}
				if (!found)
					continue;

				if (it->second.type == 2)
				{
					int selected = osd_obj->getSelected(i);
					selected++;
					if (selected == it->second.switches.size())
						selected = 0;
						
					osd_obj->setSelected(i, selected);

					runCommand(it->second.action_commands[it->second.switches.size() - selected - 1]);
				}
				else if (it->second.type == 1)
				{
					osd_obj->setSelected(i, !osd_obj->getSelected(i));
					runCommand(it->second.action_commands[0], osd_obj->getSelected(i));

				}
			
				osd_obj->selectEntry(number);
				cout << "Selected: " << i << endl;

			}
			else if (it->second.type == 0)
			{
				for (int j = 0; j < it->second.action_commands.size(); j++)
				{
					osd_obj->addCommand("HIDE menu");
					if (runCommand(it->second.action_commands[j]) == 1)
					{
						getMenu(menuNumber);						
					}
					osd_obj->addCommand("SHOW menu");
					osd_obj->addCommand("COMMAND menu select next");
				}
			}
			

			cout << "Number " << number << " selected." << endl;
		}
	} while(key != RC1_HOME && key != RC1_RIGHT && key != RC1_LEFT);
	osd_obj->addCommand("HIDE menu");

	
}
