/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "osd.h"
#include "fb.h"

int osd::start_thread()
{
	
	int status;
  
	status = pthread_create( &osdThread,
                           NULL,
                           start_osdqueue,
                           (void *)this );
	return status;

}

void* osd::start_osdqueue( void * this_ptr )
{
	osd *o = (osd *) this_ptr;

	while(1)
	{
		while(o->isEmpty())
		{	
			if (o->proginfo_shown && (time(0) > o->proginfo_hidetime))
			{
				o->hideProgramInfo();
				o->proginfo_hidetime = time(0) + 10000;
			}
			usleep(150);
		}
		o->executeQueue();
	}
}

osd::osd(settings &set, fbClass &fbclass, std::string fontpath = "/usr/lib/fonts/ds9.ttf") :setting(set), fb(fbclass)
{
	proginfo_shown = false;
	proginfo_hidetime = time(0) + 10000;
	printf("OSD\n");
	fb.setMode(720, 576, 16);
	fb.clearScreen();
	fb.loadFonts(fontpath);

	if (setting.boxIsGTX())
	{
		fb.setFade(1, 22, 5, 57, 63, 63, 63);
		fb.setFade(2, 24, 5, 59, 63, 63, 63);
		fb.setFade(0, 1, 1, 1, 60, 60, 0);
		fb.setFade(3, 0, 0, 63, 0, 0, 0);
		fb.setFade(4, 0, 0, 63, 63, 63, 63);
		fb.setFade(5, 22, 5, 57, 60, 60, 0);
		fb.setFade(6, 22, 5, 57, 0, 60, 0);
		fb.setFade(7, 1, 1, 1, 63, 63, 63);
		fb.setFade(8, 60, 60, 0, 1, 1, 1);
	}
	else
	{
		fb.setFade(1, 62, 5, 18, 63, 63, 63);
		fb.setFade(2, 64, 5, 20, 63, 63, 63);
		fb.setFade(0, 1, 1, 1, 0, 60, 60);
		fb.setFade(3, 63, 0, 0, 1, 1, 1);
		fb.setFade(4, 63, 0, 0, 63, 63, 63);
		fb.setFade(5, 62, 5, 18, 1, 1, 1);
		fb.setFade(6, 62, 5, 18, 0, 60, 0);
		fb.setFade(7, 1, 1, 1, 63, 63, 63);
		fb.setFade(8, 0, 60, 60, 1, 1, 1);
	}
	for (int i = 0; i <= 17; i++)
	{
		circle[i] = 17 - (int) ((sqrt((float)1 - (((float)i/17) * ((float)i / 17)))) * 17);
		
	}
	for (int i = 0; i <= 10; i++)
	{
		circlesmall[i] = 10 - (int) ((sqrt((float)1 - (((float)i/10) * ((float)i / 10)))) * 10);
		
	}
	for (int i = 0; i <= 12; i++)
	{
		circlemiddle[i] = 12 - (int) ((sqrt((float)1 - (((float)i/12) * ((float)i / 12)))) * 12);
		
	}
}

void osd::addCommand(std::string command)
{
	command_queue.push(command);
	printf("Command: %s\n", command.c_str());
}

void osd::executeQueue()
{
	while(!isEmpty())
	{
		executeCommand();
	}
}

void osd::executeCommand()
{
	std::string command_from_queue = command_queue.front();
	std::string command;

	command_queue.pop();

	std::istringstream iss(command_from_queue);
	std::getline(iss, command, ' ');

	if(command == "CLEARSCREEN")
	{
		clearScreen();
	}
	else if (command == "CREATE")
	{
		std::string command2;
		std::getline(iss, command2, ' ');
		if (command2 == "list")
		{
			createList();
		}
		else if (command2 == "proginfo")
		{
			createProgramInfo();
		}
		else if (command2 == "epg")
		{
			createEPG();
		}
		else if (command2 == "number")
		{
			createNumberEntry();
		}
		else if (command2 == "perspective")
		{
			createPerspective();
		}
		else if (command2 == "menu")
		{
			createMenu();
		}
		else if (command2 == "scan")
		{
			createScan();
		}
		else if (command2 == "schedule")
		{
			createSchedule();
		}
		else if (command2 == "ip")
		{
			createIP();
		}
	}
	else if (command == "COMMAND")
	{
		std::string command2;
		std::getline(iss, command2, ' ');
		std::string parms[10];
		int parm_count = 0;

		while(std::getline(iss, parms[parm_count++], ' '));

		if (command2 == "list")
		{
			if (parms[0] == "add_item")
			{
				addListItem(atoi(parms[1].c_str()), parms[2]);
			}
			else if (parms[0] == "select_item")
			{
				selectItem(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "select_next_item")
			{
				selectNextItem();
			}
		}
		else if (command2 == "proginfo")
		{
			if (parms[0] == "set_service_name")
			{
				std::string name;
				std::istringstream iss(command_from_queue);
				std::getline(iss, name, ' ');
				std::getline(iss, name, ' ');
				std::getline(iss, name, ' ');
				std::getline(iss, name);

				setServiceName(name);
			}
			else if (parms[0] == "set_language_name")
			{
				setLanguage(parms[1]);
			}
			else if (parms[0] == "set_service_number")
			{
				setServiceNumber(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "set_red_available")
			{
				if (parms[1] == "true")
					setRedAvailable(true);
				else
					setRedAvailable(false);
			}
			else if (parms[0] == "set_green_available")
			{
				if (parms[1] == "true")
					setGreenAvailable(true);
				else
					setGreenAvailable(false);
			}
			else if (parms[0] == "set_yellow_available")
			{
				if (parms[1] == "true")
					setYellowAvailable(true);
				else
					setYellowAvailable(false);
			}
			else if (parms[0] == "set_blue_available")
			{
				if (parms[1] == "true")
					setBlueAvailable(true);
				else
					setBlueAvailable(false);
			}
			else if (parms[0] == "set_teletext")
			{
				if (parms[1] == "true")
					setTeletext(true);
				else
					setTeletext(false);
			}
			else if (parms[0] == "set_now_starttime")
			{
				setNowTime(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "set_now_description")
			{
				setNowDescription(parms[1]);
			}
			else if (parms[0] == "set_next_starttime")
			{
				setNextTime(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "set_next_description")
			{
				setNextDescription(parms[1]);
			}
			else if (parms[0] == "set_parental_rating")
			{
				setParentalRating(atoi(parms[1].c_str()));
			}
		}
		else if (command2 == "epg")
		{
			if (parms[0] == "set_event_name")
			{
				setEPGEventName(parms[1]);
			}
			else if (parms[0] == "set_short_text")
			{
				setEPGEventShortText(parms[1]);
			}
			else if (parms[0] == "set_extended_text")
			{
				setEPGEventExtendedText(parms[1]);
			}
			else if (parms[0] == "set_description")
			{
				setEPGDescription(parms[1]);
			}
			else if (parms[0] == "set_service_name")
			{
				setEPGProgramName(parms[1]);
			}
			else if (parms[0] == "set_start_time")
			{
				setEPGstarttime(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "set_duration")
			{
				setEPGduration(atoi(parms[1].c_str()));
			}
		}
		else if (command2 == "number")
		{
			if (parms[0] == "number")
			{
				setNumberEntry(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "text")
			{
				setNumberText(parms[1]);
			}
		}
		else if (command2 == "perspective")
		{
			if (parms[0] == "name")
			{
				setPerspectiveName(parms[1]);
			}
		}
		else if (command2 == "menu")
		{
			if (parms[0] == "add_entry")
			{
				addMenuEntry(atoi(parms[1].c_str()), parms[2], atoi(parms[3].c_str()));
			}
			else if (parms[0] == "add_switch_param")
			{
				addSwitchParameter(atoi(parms[1].c_str()), parms[2]);
			}
			else if (parms[0] == "set_size")
			{
				setMenuSize(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "set_selected")
			{
				setSelected(atoi(parms[1].c_str()), atoi(parms[2].c_str()));
			}
			else if (parms[0] == "menu_title")
			{
				setMenuTitle(parms[1]);
			}
			else if (parms[0] == "draw_entry")
			{
				if (parms[2] == "true")
					drawMenuEntry(atoi(parms[1].c_str()), true);
				else
					drawMenuEntry(atoi(parms[1].c_str()), false);
			}
			else if (parms[0] == "select")
			{
				if (parms[1] == "next")
					selectNextEntry();
				else if (parms[1] == "prev")
					selectPrevEntry();
				else
					selectEntry(atoi(parms[1].c_str()));
			}
		}
		else if (command2 == "vol")
		{
			if (parms[0] == "set")
			{
				setVolume(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "mute")
			{
				if (parms[1] == "true")
					setMute(true);
				else
					setMute(false);
			}
		}
		else if (command2 == "scan")
		{
			if (parms[0] == "progress")
			{
				setScanProgress(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "number_channels")
			{
				setScanChannelNumber(atoi(parms[1].c_str()));
			}
		}
		else if (command2 == "schedule")
		{
			if (parms[0] == "add_information")
			{
			}
			else if (parms[0] == "select")
			{
				if (parms[1] == "next")
					selectNextScheduleInformation();
				else if (parms[1] == "prev")
					selectPrevScheduleInformation();
				else
				{
					if (parms[2] == "true")
						selectScheduleInformation(atoi(parms[1].c_str()), true);
					else
						selectScheduleInformation(atoi(parms[1].c_str()), false);
				}
			}
			else if (parms[0] == "page")
			{
				if (parms[1] == "next")
					nextSchedulePage();
				else if (parms[1] == "prev")
					prevSchedulePage();
			}
		}
		else if (command2 == "ip")
		{
			if (parms[0] == "set")
			{
				setIP(atoi(parms[1].c_str()));
			}
			else if (parms[0] == "draw_position")
			{
				drawIPPosition(atoi(parms[1].c_str()), atoi(parms[2].c_str()));
			}
			else if (parms[0] == "description")
			{
				setIPDescription(parms[1]);	
			}
			else if (parms[0] == "position")
			{
				if (parms[1] == "next")
					setIPNextPosition();
				else if (parms[1] == "prev")
					setIPPrevPosition();
				else
					setIPPosition(atoi(parms[1].c_str()));
			}
		}
	}
	else if (command == "SHOW")
	{
		std::string command2;
		std::getline(iss, command2, ' ');
		
		if (proginfo_shown && (command2 != "proginfo"))
		{
			hideProgramInfo();
		}
		
		if (command2 == "list")
		{
			showList();
		}
		else if (command2 == "proginfo")
		{
			std::string duration;
			std::getline(iss, duration, ' ');
			showProgramInfo();
			proginfo_hidetime = time(0) + atoi(duration.c_str());
		}
		else if (command2 == "epg")
		{
			showEPG();
		}
		else if (command2 == "number")
		{
			showNumberEntry();
		}
		else if (command2 == "perspective")
		{
			showPerspective();
		}
		else if (command2 == "menu")
		{
			showMenu();
		}
		else if (command2 == "vol")
		{
			showVolume();
		}
		else if (command2 == "scan")
		{
			showScan();
			printf("+-+-+-+-+-+ Aufruf Channelscan\n");
		}
		else if (command2 == "schedule")
		{
			std::string page;
			std::getline(iss, page, ' ');
			showSchedule(atoi(page.c_str()));
		}
		else if (command2 == "ip")
		{
			showIP();
		}
		else if (command2 == "about")
		{
			showAbout();
		}

	}
	else if (command == "HIDE")
	{
		std::string command2;
		std::getline(iss, command2, ' ');
		if (command2 == "list")
		{
			hideList();
		}
		else if (command2 == "proginfo")
		{
			hideProgramInfo();
		}
		else if (command2 == "epg")
		{
			hideEPG();
		}
		else if (command2 == "number")
		{
			hideNumberEntry();
		}
		else if (command2 == "perspective")
		{
			hidePerspective();
		}
		else if (command2 == "menu")
		{
			hideMenu();
		}
		else if (command2 == "vol")
		{
			hideVolume();
		}
		else if (command2 == "scan")
		{
			hideScan();
		}
		else if (command2 == "schedule")
		{
			hideSchedule();
		}
		else if (command2 == "ip")
		{
			hideIP();
		}
		else if (command2 == "about")
		{
			hideAbout();
		}
	}
	else
	{
		printf("--------->UNKNOWN OSD-COMMAND<----------\n");
	}
		
}

void osd::clearScreen()
{
	fb.clearScreen();
}

void osd::createList()
{

	
	selected = 0;
	numberItems = 0;

}

int osd::numberPossibleListItems()
{
	return 10;
}

void osd::addListItem(int index, std::string  name)
{
	list[numberItems].index = index;
	list[numberItems].name = name;
	numberItems++;
}

void osd::selectNextItem()
{
}

void osd::selectItem(int number)
{
	char buffer[100];
	fb.setTextSize(0.4);
	for (int j = 0; j <= 17; j++)
	{
		fb.fillBox(33 + circle[17 - j], 85 + selected * 40 + j, 50, 85 + selected * 40 + j + 1, 1);
	}
	for (int j = 18; j < 35; j++)
	{
		fb.fillBox(33 + circle[j - 18], 85 + selected * 40 + j, 50, 85 + selected * 40 + j + 1, 1);
	}
	fb.fillBox(33, 103 + selected * 40, 50, 104 + selected * 40, 1);
	fb.fillBox(50, 85 + selected * 40, 70, 120 + selected * 40, 1);
	fb.fillBox(300, 85 + selected * 40, 320, 120 + selected * 40, 1);
	fb.fillBox(70, 85 + selected * 40, 300, 120 + selected * 40, 0);
	strcpy(buffer, list[selected].name.c_str());
	fb.putText(75, 110 + selected * 40, 0, buffer, 225);
	selected = number;
	for (int j = 0; j <= 17; j++)
	{
		fb.fillBox(33 + circle[17 - j], 85 + selected * 40 + j, 50, 85 + selected * 40 + j + 1, 0);
	}
	for (int j = 18; j < 35; j++)
	{
		fb.fillBox(33 + circle[j - 18], 85 + selected * 40 + j, 50, 85 + selected * 40 + j + 1, 0);
	}
	fb.fillBox(33, 103 + selected * 40, 50, 104 + selected * 40, 0);
	fb.fillBox(50, 85 + selected * 40, 70, 120 + selected * 40, 0);
	fb.fillBox(300, 85 + selected * 40, 320, 120 + selected * 40, 0);
	fb.fillBox(70, 85 + selected * 40, 300, 120 + selected * 40, 1);
	strcpy(buffer, list[selected].name.c_str());
	fb.putText(75, 110 + selected * 40, 1, buffer, 225);
}

int osd::selectedItem()
{
	return selected;
}

void osd::showList()
{
	char buffer[100];

	fb.setTextSize(0.4);
	for (int i = 0; i < numberItems; i++)
	{
		for (int j = 0; j <= 17; j++)
		{
			fb.fillBox(33 + circle[17 - j], 85 + i * 40 + j, 50, 85 + i * 40 + j + 1, 1);
		}
		for (int j = 18; j < 35; j++)
		{
			fb.fillBox(33 + circle[j - 18], 85 + i * 40 + j, 50, 85 + i * 40 + j + 1, 1);
		}
		fb.fillBox(33, 102 + selected * 40, 50, 106 + selected * 40, 1);
		fb.fillBox(50, 85 + i * 40, 70, 120 + i * 40, 1);
		fb.fillBox(300, 85 + i * 40, 320, 120 + i * 40, 1);
		fb.fillBox(323, 85 + i * 40, 330, 120 + i * 40, 2);
		fb.fillBox(70, 85 + i * 40, 300, 120 + i * 40, 0);
		strcpy(buffer, list[i].name.c_str());
		fb.putText(75, 110 + i * 40, 0, buffer, 225);
	}
}

void osd::hideList()
{
	fb.fillBox(30, 85, 330, 120 + 9 * 40, -1);
}


void osd::createProgramInfo()
{
	serviceName = "";
	serviceNumber = 0;
	redAvailable = false;
	greenAvailable = false;
	yellowAvailable = false;
	blueAvailable = false;
	teletext = false;
	nowTime = 0;
	nextTime = 0;
	nowDescription = "";
	nextDescription = "";
	language[0] = '\0';
	par_rating = 0;
}

void osd::setServiceName(std::string  name)
{
	serviceName = name;
	if (proginfo_shown)
	{
		fb.fillBox(130, 30, 330, 50, 0);
		fb.putText(135, 47, 0, serviceName, 195);
	}
}

void osd::setServiceNumber(int number)
{
	serviceNumber = number;
	if (proginfo_shown)
	{
		fb.fillBox(75, 30, 120, 50, 0);
		fb.putText(77, 45, 0, serviceNumber, 43);
	}
}

void osd::setRedAvailable(bool available)
{
	redAvailable = available;
}

void osd::setTeletext(bool available)
{
	teletext = available;
	if (proginfo_shown)
	{
		if (teletext)
		{
			fb.fillBox(80, 401, 120, 419, 0);
			fb.putText(82, 415, 0, "TXT");
		}
		else
			fb.fillBox(80, 401, 120, 419, 1);
	}
}

void osd::setGreenAvailable(bool available)
{
	greenAvailable = available;
}

void osd::setYellowAvailable(bool available)
{
	yellowAvailable = available;
}

void osd::setBlueAvailable(bool available)
{
	blueAvailable = available;
}

void osd::setNowTime(time_t starttime)
{
	
	nowTime = starttime;
	if (proginfo_shown)
	{
		char nowtime[10];
		struct tm *t;
		t = localtime(&nowTime);
		strftime(nowtime, sizeof nowtime, "%H:%M", t);
		
		fb.fillBox(65, 425, 125, 445, 0);
		fb.putText(70, 440, 0, nowtime); // Uhrzeit des laufenden Programms

	}
}

void osd::setNowDescription(std::string  description)
{
	nowDescription = description;
	if (proginfo_shown)
	{
		fb.fillBox(149, 425, 650, 445, 0);
		fb.putText(150, 440, 0, nowDescription, 490);
	}
}

void osd::setNextTime(time_t starttime)
{
	nextTime = starttime;
	if (proginfo_shown)
	{
		char nexttime[10];
		struct tm *t;
		t = localtime(&nextTime);
		strftime(nexttime, sizeof nexttime, "%H:%M", t);
		
		fb.fillBox(65, 495, 125, 515, 0);
		fb.putText(70, 510, 0, nexttime); // Uhrzeit des nächsten Programms
	}
}

void osd::setNextDescription(std::string  description)
{
	nextDescription = description;
	if (proginfo_shown)
	{
		fb.fillBox(149, 495, 650, 515, 0);
		fb.putText(150, 510, 0, nextDescription, 490); // Erste Zeile des nächsten Programms
	}
}

void osd::setLanguage(std::string  language_name)
{
	strcpy(language, language_name.c_str());
	if (proginfo_shown)
	{
		if (language_name != "")
		{
			fb.fillBox(160, 401, 300, 419, 0);
			fb.putText(165, 415, 0, language);
		}
		else
			fb.fillBox(160, 401, 300, 419, 1);
	}
}

void osd::setParentalRating(int rating)
{
	par_rating = rating;
	if (proginfo_shown)
	{
		if (par_rating != 0)
		{
			fb.fillBox(310, 401, 390, 419, 0);
			char rattext[10];
			sprintf(rattext, "FSK: %d", par_rating + 3);
			fb.putText(315, 415, 0, rattext);
		}
		else
			fb.fillBox(310, 401, 390, 419, 1);
	}
}

void osd::showProgramInfo()
{
	if (proginfo_shown == true)
		return;
	proginfo_shown = true;

	printf("%d - %s\n", serviceNumber, serviceName.c_str());
	printf("Now (%d): %s\n", (int)nowTime, nowDescription.c_str());
	printf("Next (%d): %s\n", (int)nextTime, nextDescription.c_str());
	if (redAvailable)
		printf("Red ");
	
	if (yellowAvailable)
		printf("Yellow ");
	if (blueAvailable)
		printf("Blue ");
	printf("\n");

	char nowtime[10], nexttime[10], acttime[10], number[10];
	char pname[30], now[200], next[200];

	fb.fillBox(65, 420, 650, 550, 0);
	for (int j = 0; j <= 10; j++)
	{
		fb.fillBox(60 + circlesmall[10 - j], 30 + j, 70, 30 + j + 1, 1);
		fb.fillBox(60 + circlesmall[10 - j], 400 + j, 70, 400 + j + 1, 1);
		fb.fillBox(650, 400 + j, 660 - circlesmall[10 - j], 400 + j + 1, 1);
		fb.fillBox(650 - circlesmall[10 - j], 420 + j, 650, 420 + j + 1, 1);
		fb.fillBox(130 - circlesmall[10 - j], 490 + j, 130, 490 + j + 1, 1);
		fb.fillBox(130 - circlesmall[10 - j], 420 + j, 130, 420 + j + 1, 1);
		fb.fillBox(650 - circlesmall[10 - j], 490 + j, 650, 490 + j + 1, 1);
		fb.fillBox(650 - circlesmall[j], 470 + j, 650, 470 + j + 1, 1);
		fb.fillBox(130 - circlesmall[j], 470 + j, 130, 470 + j + 1, 1);
		fb.fillBox(140, 470 + j, 140 + circlesmall[j], 470 + j + 1, 1);
		fb.fillBox(140, 490 + j, 140 + circlesmall[10 - j], 490 + j + 1, 1);
		fb.fillBox(140, 420 + j, 140 + circlesmall[10 - j], 420 + j + 1, 1);
	}
	for (int j = 11; j < 20; j++)
	{
		fb.fillBox(60 + circlesmall[j - 11], 30 + j, 70, 30 + j + 1, 1);
		fb.fillBox(60 + circlesmall[j - 11], 400 + j, 70, 400 + j + 1, 1);
		
	}
	fb.fillBox(70, 30, 75, 50, 1);
	fb.fillBox(75, 30, 120, 50, 0);
	fb.fillBox(120, 30, 130, 50, 1);
	fb.fillBox(130, 30, 330, 50, 0);
	fb.fillBox(330, 30, 550, 50, 1);
	fb.fillBox(550, 30, 630, 50, 0);
	fb.fillBox(630, 30, 670, 50, 1);

	fb.fillBox(70, 400, 650, 420, 1);
	fb.fillBox(650, 410, 660, 550, 1);
	fb.fillBox(70, 480, 660, 490, 1);
	fb.fillBox(130, 420, 140, 550, 1);
	
	fb.setTextSize(0.4);
	
	strcpy(pname, serviceName.c_str());
	fb.putText(135, 47, 0, pname, 195);

	sprintf(number, "%d", serviceNumber);
	fb.putText(77, 45, 0, number, 43);

	time_t act_time = time(0);
	struct tm *t;
	t = localtime(&act_time);
	strftime(acttime, sizeof acttime, "%H:%M", t);
	fb.putText(560, 45, 0, acttime); // aktuelle Uhrzeit
	
	t = localtime(&nowTime);
	strftime(nowtime, sizeof nowtime, "%H:%M", t);
	t = localtime(&nextTime);
	strftime(nexttime, sizeof nexttime, "%H:%M", t);
	fb.putText(70, 440, 0, nowtime); // Uhrzeit des laufenden Programms
	fb.putText(70, 510, 0, nexttime); // Uhrzeit des nächsten Programms

	strcpy(now, nowDescription.c_str());
	strcpy(next, nextDescription.c_str());

	if (strlen(language) > 0)
	{
		fb.fillBox(160, 401, 300, 419, 0);
		fb.putText(165, 415, 0, language);
	}

	if (teletext)
	{
		fb.fillBox(80, 401, 120, 419, 0);
		fb.putText(82, 415, 0, "TXT");
	}

	if (par_rating != 0)
	{
		fb.fillBox(310, 401, 390, 419, 0);
		char rattext[10];
		sprintf(rattext, "FSK: %d", par_rating + 3);
		fb.putText(315, 415, 0, rattext);
	}
	
	if (greenAvailable)
		fb.putText(600, 415, 6, "O");
	
	fb.putText(150, 440, 0, now, 490); // Erste Zeile des aktuellen Programms

	fb.putText(150, 510, 0, next, 490); // Erste Zeile des nächsten Programms
	
	char text[100];
	sprintf(text, "CAID: %x", setting.getCAID());

}

void osd::hideProgramInfo()
{
	proginfo_shown = false;
	printf("Hiding program info\n");
	fb.fillBox(30, 29, 670, 52, -1);
	fb.fillBox(30, 380, 730, 670, -1);
	
}

void osd::createNumberEntry()
{
	number = -1;
	numberText = "";

}

void osd::setNumberEntry(int setnumber)
{
	number = setnumber;
}

void osd::setNumberText(std::string  text)
{
	numberText = text;
}

void osd::showNumberEntry()
{
	char buffer[100];

	fb.setTextSize(0.7);
	for (int j = 0; j <= 17; j++)
	{
		fb.fillBox(33 + circle[17 - j], 30 + j, 50, 30 + j + 1, 1);
		fb.fillBox(320, 30 + j, 338 - circle[17 - j], 30 + j + 1, 1);
	}
	for (int j = 18; j < 35; j++)
	{
		fb.fillBox(33 + circle[j - 18], 30 + j, 50, 30 + j + 1, 1);
		fb.fillBox(320, 30 + j, 338 - circle[j - 18], 30 + j + 1, 1);
	}
	for (int j = 0; j <= 10; j++)
	{
		fb.fillBox(33 + circlesmall[10 - j], 75 + j, 43, 75 + j + 1, 1);
		fb.fillBox(320, 75 + j, 330 - circlesmall[10 - j], 75 + j + 1, 1);
	
	}
	for (int j = 11; j < 20; j++)
	{
		fb.fillBox(33 + circlesmall[j - 11], 75 + j, 43, 75 + j + 1, 1);
		fb.fillBox(320, 75 + j, 330 - circlesmall[j - 11], 75 + j + 1, 1);
		
	}
	fb.fillBox(50, 30, 75, 65, 1);
	fb.fillBox(75, 30, 125, 65, 0);
	fb.fillBox(125, 30, 135, 65, 1);
	fb.fillBox(135, 30, 185, 65, 0);
	fb.fillBox(185, 30, 195, 65, 1);
	fb.fillBox(195, 30, 245, 65, 0);
	fb.fillBox(245, 30, 255, 65, 1);
	fb.fillBox(255, 30, 305, 65, 0);
	fb.fillBox(305, 30, 320, 65, 1);

	fb.fillBox(43, 75, 320, 95, 0);

	if (number > 999)
	{
		sprintf(buffer, "%d", (int) (number / 1000) % 10);
		fb.putText(90, 60, 0, buffer);
	}
	if (number > 99)
	{
		sprintf(buffer, "%d", (int) (number / 100) % 10);
		fb.putText(150, 60, 0, buffer);
	}
	if (number > 9)
	{
		sprintf(buffer, "%d", (int) (number / 10) % 10);
		fb.putText(210, 60, 0, buffer);
	}
	if (number >= 0)
	{
		sprintf(buffer, "%d", (int) (number / 1) % 10);
		fb.putText(270, 60, 0, buffer);
	}

	fb.setTextSize(0.4);
	strcpy(buffer, numberText.c_str());
	fb.putText(45, 93, 0, buffer, 275);
}

void osd::hideNumberEntry()
{
	printf("Hiding number entry\n");
	fb.fillBox(33, 30, 350, 100, -1);
}

void osd::createPerspective()
{
	perspective_name = "";
}

void osd::setPerspectiveName(std::string  name)
{
	perspective_name = name;
}

void osd::showPerspective()
{
	for (int j = 0; j <= 10; j++)
	{
		fb.fillBox(150 + circlesmall[10 - j], 500 + j, 160, 500 + j + 1, 1);
		fb.fillBox(560, 500 + j, 570 - circlesmall[10 - j], 500 + j + 1, 1);
	
	}
	for (int j = 11; j < 20; j++)
	{
		fb.fillBox(150 + circlesmall[j - 11], 500 + j, 160, 500 + j + 1, 1);
		fb.fillBox(560, 500 + j, 570 - circlesmall[j - 11], 500 + j + 1, 1);
		
	}
	fb.fillBox(160, 500, 170, 520, 1);
	fb.fillBox(170, 500, 550, 520, 0);
	fb.fillBox(550, 500, 560, 520, 1);

	char pname[100];
	strcpy(pname, perspective_name.c_str());
	fb.setTextSize(0.4);
	fb.putText(175, 515, 0, pname, 370);
}

void osd::hidePerspective()
{
	fb.fillBox(150, 500, 580, 520, -1);
}


void osd::createEPG()
{
	event_name = "";
	event_short_text = "";
	event_extended_text = "";
	description = "";
}

void osd::setEPGEventName(std::string  input)
{
	event_name = input;
}

void osd::setEPGEventShortText(std::string  input)
{
	event_short_text = input;
}

void osd::setEPGEventExtendedText(std::string  input)
{
	event_extended_text = input;
}

void osd::setEPGDescription(std::string  input)
{
	description = input;
}

void osd::setEPGProgramName(std::string  input)
{
	programname = input;
}

void osd::setEPGstarttime(time_t input)
{
	starttime = input;
}

void osd::setEPGduration(int input)
{
	duration = input;
}

void osd::showEPG()
{
	char text[1000];

	for (int j = 0; j <= 12; j++)
	{
		fb.fillBox(88 + circlemiddle[12 - j], 45 + j, 100, 45 +  j + 1, 5);
		fb.fillBox(88 + circlemiddle[j], 57 + j, 100, 57 +  j + 1, 5);
		fb.fillBox(600, 45 + j, 612 - circlemiddle[12 - j], 45 +  j + 1, 5);
		
	}
	fb.fillBox(100, 45, 120, 69, 5);
	fb.fillBox(120, 45, 480, 69, 7);
	fb.fillBox(480, 45, 490, 69, 5);
	fb.fillBox(490, 45, 560, 69, 7);
	fb.fillBox(560, 45, 600, 69, 5);
	fb.setTextSize(0.45);
	fb.putText(125, 63, 7, event_name, 350);
	time_t act_time = time(0);
	struct tm *t;
	t = localtime(&act_time);
	strftime(text, 6, "%H:%M", t);
	fb.putText(495, 63, 7, text);

	fb.fillBox(600, 57, 612, 500, 5);

	fb.setTextSize(0.4);
	fb.fillBox(100, 75, 120, 99, 5);
	fb.fillBox(120, 75, 400, 99, 0);
	fb.fillBox(400, 75, 410, 99, 5);
	fb.fillBox(410, 75, 480, 99, 0);
	fb.fillBox(480, 75, 490, 99, 5);
	fb.fillBox(490, 75, 560, 99, 0);
	fb.fillBox(560, 75, 580, 99, 5);
	fb.putText(125, 93, 0, programname, 270);
				
	t = localtime(&starttime);
	strftime(text, 6, "%H:%M", t);
	fb.putText(415, 93, 0, text);

	starttime += duration;
	t = localtime(&starttime);
	strftime(text, 6, "%H:%M", t);
	fb.putText(495, 93, 0, text);

	fb.setTextSize(0.45);
	fb.fillBox(100, 105, 120, 129, 5);
	fb.fillBox(120, 105, 560, 129, 7);
	fb.fillBox(560, 105, 600, 129, 5);
	fb.putText(125, 123, 7, event_short_text, 430);

	fb.fillBox(100, 135, 110, 470, 5);
	fb.fillBox(110, 135, 580, 470, 7);
	fb.fillBox(580, 135, 590, 470, 5);

	nlcounter = 0;
	int last = 0;
	int length = 0;
	printf("Start\n");
	for (int i = 0; i < (int) event_extended_text.length(); i++)
	{
		if (event_extended_text[i] == ' ')
		{
			last = i;
			length += fb.getWidth(event_extended_text[i]);
		}
		else if (event_extended_text[i] == '\n')
		{
			newlines[nlcounter++] = i;
			length = 0;
			i++;
		}
		else
		{
			
			length += fb.getWidth(event_extended_text[i]);
			if (length >= 350)
			{
				i = last;
				length = 0;
				newlines[nlcounter++] = i;
			}
		}

	}
	printf("%d\n", nlcounter);

	last = 0;
	int i;
	int stop;
	if (nlcounter > 16)
	{
		stop = 15;
	}
	else
	{
		stop = nlcounter;
	}
	for (i = 0; i < stop; i++)
	{
		printf("%d - %d\n", i, newlines[i]);
		fb.putText(115, 163 + 20 * i, 7, event_extended_text.substr(last, newlines[i] - last));
		last = newlines[i] + 1;
	}
	if (newlines[i - 1] < (int)event_extended_text.length() && nlcounter < 16)
	{
		fb.putText(115, 163 + 20 * (i), 7, event_extended_text.substr(newlines[i - 1] + 1));
	}
	printf("Ende\n");
	shown = 0;
}

void osd::showPrevEPGPage()
{
	shown--;
	if (shown < 0)
	{
		shown++;
		return;
	}
	
	fb.fillBox(100, 135, 110, 470, 5);
	fb.fillBox(110, 135, 580, 470, 7);
	fb.fillBox(580, 135, 590, 470, 5);

	int last = newlines[shown * 15];
	int i;
	int stop;
	if (nlcounter - (shown * 16) > 16)
	{
		stop = 15 * (shown + 1);
	}
	else
	{
		stop = nlcounter;
	}
	for (i = shown * 15; i < stop; i++)
	{
		printf("%d - %d\n", i, newlines[i]);
		fb.putText(115, 163 + 20 * (i - shown * 15), 7, event_extended_text.substr(last, newlines[i] - last));
		last = newlines[i] + 1;
	}
	if (newlines[i - 1] < (int)event_extended_text.length() && nlcounter - (shown * 16) < 16)
	{
		fb.putText(115, 163 + 20 * (i - shown * 15), 7, event_extended_text.substr(newlines[i - 1] + 1));
	}
}

void osd::showNextEPGPage()
{
	shown++;
	if (shown * 15 > nlcounter)
	{
		shown--;
		return;
	}
	
	fb.fillBox(100, 135, 110, 470, 5);
	fb.fillBox(110, 135, 580, 470, 7);
	fb.fillBox(580, 135, 590, 470, 5);

	int last = newlines[shown * 15];
	int i;
	int stop;
	if (nlcounter - (shown * 16) > 16)
	{
		stop = 15 * (shown + 1);
	}
	else
	{
		stop = nlcounter;
	}
	for (i = shown * 15; i < stop; i++)
	{
		printf("%d - %d\n", i, newlines[i]);
		fb.putText(115, 163 + 20 * (i - shown * 15), 7, event_extended_text.substr(last, newlines[i] - last));
		last = newlines[i] + 1;
	}
	if (newlines[i - 1] < (int)event_extended_text.length() && nlcounter - (shown * 16) < 16)
	{
		fb.putText(115, 163 + 20 * (i - shown * 15), 7, event_extended_text.substr(newlines[i - 1] + 1));
	}
}

void osd::hideEPG()
{
	fb.clearScreen();
}

void osd::createMenu()
{
	selected_entry = -1;
	number_menu_entries = 0;
	addCommand("COMMAND menu set_size 0");
}

/*
types:
0 - Normal entry (default)
1 - Select entry
2 - Switching entry
*/
void osd::addMenuEntry(int index, std::string  caption, int type = 0)
{
	menu[number_menu_entries].index = index;
	menu[number_menu_entries].caption = caption;
	menu[number_menu_entries].type = type;
	menu[number_menu_entries].switches.clear();
	number_menu_entries++;
}

void osd::addSwitchParameter(int number, std::string  parameter)
{
	menu[number].switches.insert(menu[number].switches.begin(), parameter);
}

void osd::setSelected(int number, int sel)
{
	menu[number].selected = sel;
}

void osd::setMenuTitle(std::string  title)
{
	menu_title = title;
}

void osd::selectEntry(int number)
{
	if (number_menu_entries == 0)
		return;
	if (selected_entry != -1)
		drawMenuEntry(selected_entry);
	selected_entry = number;
	drawMenuEntry(number, true);
}

void osd::selectNextEntry()
{
	if (number_menu_entries == 0)
		return;
	if (selected_entry != -1)
		drawMenuEntry(selected_entry);
	do
	{
		selected_entry++;
		if (selected_entry >= number_menu_entries)
			selected_entry = 0;
	} while(menu[selected_entry].type == 3);
	drawMenuEntry(selected_entry, true);
}

void osd::selectPrevEntry()
{
	if (number_menu_entries == 0)
		return;
	if (selected_entry != -1)
		drawMenuEntry(selected_entry);
	do
	{
		selected_entry--;
		if (selected_entry < 0)
			selected_entry = number_menu_entries - 1;
	} while(menu[selected_entry].type == 3);
	drawMenuEntry(selected_entry, true);
}

int osd::menuSelectedIndex()
{
	return menu[selected_entry].index;
}

void osd::drawMenuEntry(int number, bool selected = false)
{
	int color1 = 5;
	int color2 = 0;
	int color3 = 6;
	if (selected)
	{
		color1 = 0;
		color2 = 5;
		color3 = 7;
	}
	if (menu[number].type != 3)
	{
		fb.fillBox(100, 75 + number * 30, 110, 100 + number * 30, color1);
		fb.fillBox(110, 75 + number * 30, 180, 100 + number * 30, color2);
		fb.fillBox(180, 75 + number * 30, 440 + menu_size, 100 + number * 30, color1);
		fb.putText(190, 95 + number * 30, color1, menu[number].caption, 250 + menu_size);
		fb.putText(125, 95 + number * 30, color2, menu[number].index);
		if (menu[number].type == 1)
		{
			fb.fillBox(420, 80 + number * 30, 435, 95 + number * 30, color2);
			if (!menu[number].selected)
				fb.fillBox(423, 83 + number * 30, 432, 92 + number * 30, color1);
		}
		if (menu[number].type == 2)
		{
			fb.putText(435, 95 + number * 30, color3, menu[number].switches[menu[number].selected], -1, 1);
		}
	}
	else
	{
		fb.fillBox(100, 75 + number * 30, 452, 100 + number * 30, color1);
		fb.putText(130, 95 + number * 30, color1, menu[number].caption);
	}
}

void osd::showMenu()
{
	fb.setTextSize(0.45);
	fb.fillBox(100, 45, 452 + menu_size, 70, 5);
	fb.fillBox(452 + menu_size, 57, 464 + menu_size, 75 + number_menu_entries * 30, 5);
	fb.fillBox(125, 45, 350, 70, 0);
	fb.putText(130, 65, 0, menu_title);
	for (int j = 0; j <= 12; j++)
	{
		fb.fillBox(88 + circlemiddle[12 - j], 45 + j, 100, 45 +  j + 1, 5);
		fb.fillBox(88 + circlemiddle[j], 57 + j, 100, 57 +  j + 1, 5);
		fb.fillBox(452 + menu_size, 45 + j, 464 - circlemiddle[12 - j] + menu_size, 45 +  j + 1, 5);
	}

	for (int i = 0; i < number_menu_entries; i++)
	{
		drawMenuEntry(i);
	}
}

void osd::hideMenu()
{
	fb.fillBox(88, 45, 464 + menu_size, 600, -1);
}

void osd::setMute(bool mute)
{
	if (mute)
	{
		for (int j = 0; j <= 12; j++)
		{
			fb.fillBox(500 + circlemiddle[12 - j], 75 + j, 513, 75 +  j + 1, 5);
			fb.fillBox(500 + circlemiddle[j], 87 + j, 513, 87 +  j + 1, 5);
			fb.fillBox(585, 75 + j, 598 - circlemiddle[12 - j], 75 +  j + 1, 5);
			fb.fillBox(585, 87 + j, 598 - circlemiddle[j], 87 +  j + 1, 5);

		}
		fb.fillBox(513, 75, 585, 99, 0);
		fb.setTextSize(0.45);
		fb.putText(515, 93, 0, "Silence");
		
	}
	else
	{
		fb.fillBox(500, 75, 598, 100, -1);
	}
}

void osd::showVolume()
{
	for (int j = 0; j <= 10; j++)
	{
		fb.fillBox(500 + circlesmall[10 - j], 110 + j, 510, 110 + j + 1, 5);
		fb.fillBox(500 + circlesmall[j], 120 + j, 510, 120 + j + 1, 5);
		fb.fillBox(500 + circlesmall[10 - j], 125 + j, 510, 125 + j + 1, 5);
		fb.fillBox(500 + circlesmall[j], 135 + j, 510, 135 + j + 1, 5);

		fb.fillBox(588, 110 + j, 598 - circlesmall[10 - j], 110 + j + 1, 5);
		fb.fillBox(588, 120 + j, 598 - circlesmall[j], 120 + j + 1, 5);
		fb.fillBox(588, 125 + j, 598 - circlesmall[10 - j], 125 + j + 1, 5);
		fb.fillBox(588, 135 + j, 598 - circlesmall[j], 135 + j + 1, 5);
	}
	fb.fillBox(510, 110, 588, 145, 0);

	int slider = (int) (((float)volume / 63) * 10);

	for (int i = 0; i < slider; i++)
	{
		fb.fillBox(515 + i * 7, 115, 519 + i * 7, 125, 8);
		fb.fillBox(515 + i * 7, 130, 519 + i * 7, 140, 8);
	}

	for (int i = slider + 1; i < 10; i++)
	{
		fb.fillBox(515 + i * 7, 115, 519 + i * 7, 125, 0);
		fb.fillBox(515 + i * 7, 130, 519 + i * 7, 140, 0);
	}
}

void osd::hideVolume()
{
	fb.fillBox(500, 110, 598, 145, -1);
}

void osd::createScan()
{
	percentage = 0;
	channel_count = 0;
	fb.setTextSize(0.5);
}

void osd::setScanProgress(int percent)
{
	percentage = percent;

	for (int i = 0; i < (int) ((float)percentage / 5); i++)
	{
		fb.fillBox(225 + i * 13, 465, 231 + i * 13, 485, 8);
	}

	for (int i = (int) ((float)percentage / 5) + 1; i < 20; i++)
	{
		fb.fillBox(225 + i * 13, 465, 231 + i * 13, 485, 0);
	}


	
	char perc[5];
	
	sprintf(perc, "%d", percentage);
	strcat(perc, " %");

	fb.fillBox(330, 430, 400, 459, 5);
	fb.putText(330, 450, 5, perc);
}

void osd::setScanChannelNumber(int number)
{
	channel_count = number;

	
	fb.fillBox(410, 350, 500, 380, 5);
	fb.putText(480, 370, 5, channel_count, -1, 1);

}

void osd::showScan()
{
	printf("+-+-+-+-+ Draw Channelscan\n");
	fb.fillBox(200, 300, 500, 500, 5);
	fb.fillBox(210, 460, 490, 490, 0);
	fb.putText(300, 320, 5, "Channel-Scan");
	fb.fillBox(200, 325, 500, 327, 0);
	fb.fillBox(200, 400, 500, 402, 0);

	fb.putText(220, 370, 5, "Found Channels:");
}

void osd::hideScan()
{
	fb.fillBox(200, 300, 500, 500, -1);
}

void osd::createSchedule()
{
	sched.clear();
	selected_sched = 0;
}

void osd::addScheduleInformation(time_t starttime, std::string description, int eventid)
{
	printf("Add: %s\n", description.c_str());
	scheduling tmp_sched;
	tmp_sched.starttime = starttime;
	tmp_sched.description = description;
	tmp_sched.eventid = eventid;
	sched.insert(sched.end(), tmp_sched);
}

void osd::selectScheduleInformation(int select, bool redraw = true)
{
	printf("Selektiere %d von %d\n", select, selected_sched);
	if (redraw)
		printf("With redraw\n");
	else
		printf("Without redraw\n");

	if (sched.size() == 0)
		return;
	
	char text[20];
	struct tm *t;

	if (redraw)
	{
		fb.fillBox(100, 103 + selected_sched * 20, 630, 124 + selected_sched * 20, 5);

		t = localtime(&(sched[shown_page * 15 + selected_sched].starttime));
		strftime(text, 11, "%a, %H:%M", t);
		fb.putText(105, 120 + selected_sched * 20, 5, text);
		fb.putText(210, 120 + selected_sched * 20, 5, sched[shown_page * 15 + selected_sched].description, 410);
	}

	selected_sched = select;

	fb.fillBox(103, 104 + selected_sched * 20, 204, 124 + selected_sched * 20, 0);
	fb.fillBox(207, 104 + selected_sched * 20, 625, 124 + selected_sched * 20, 0);
	t = localtime(&(sched[shown_page * 15 + selected_sched].starttime));
	strftime(text, 11, "%a, %H:%M", t);
	fb.putText(105, 120 + selected_sched * 20, 0, text);
	fb.putText(210, 120 + selected_sched * 20, 0, sched[shown_page * 15 + selected_sched].description, 410);


}

int osd::numberSchedulePages()
{
	printf("9\n");
	return (int) ((float)sched.size() / 15) + 1;
}

void osd::showSchedule(int page)
{
	printf("Begin showing\n");
	shown_page = page;
	printf("1\n");
	if (shown_page < 0)
	{
		printf("2\n");
		shown_page = 0;
		return;
	}
	else if (shown_page >= numberSchedulePages())
	{
		printf("3\n");
		shown_page = numberSchedulePages() - 1;
		printf("10\n");
		return;
	}
	
	fb.fillBox(100, 100, 630, 420, 5);
	printf("4\n");
	int max = 15;
	if ((page + 1) * 15 > (int)sched.size())
		max = sched.size() % 15;
	printf("Page: %d - Sched.size: %d\n", page, sched.size());
	fb.setTextSize(0.4);
	printf("Max: %d\n", max);
	for (int i = 0; i < max; i++)
	{
		
		char text[20];
		struct tm *t;
		t = localtime(&(sched[page * 15 + i].starttime));
		strftime(text, 11, "%a, %H:%M", t);
		fb.putText(105, 120 + i * 20, 5, text);
		fb.putText(210, 120 + i * 20, 5, sched[page * 15 + i].description, 410);
	}
	printf("End showing\n");
}

void osd::hideSchedule()
{
	fb.fillBox(100, 100, 630, 420, -1);
}

void osd::selectNextScheduleInformation()
{

	int max = 15;
	if ((shown_page + 1) * 15 > (int)sched.size())
		max = sched.size() % 15;

	printf("Max: %d - Selected_sched: %d - shown_page: %d - numberSchedulePages: %d\n", max, selected_sched, shown_page, numberSchedulePages());

	if (selected_sched + 1 < max)
		selectScheduleInformation(selected_sched + 1);
	else
	{
		if (shown_page + 1 >= numberSchedulePages())
			return;
		showSchedule(shown_page + 1);
		selectScheduleInformation(0, false);
	}
}

void osd::selectPrevScheduleInformation()
{
	int max = 15;
	if ((shown_page + 1) * 15 > (int)sched.size())
		max = sched.size() % 15;
	
	if (selected_sched - 1 > -1)
		selectScheduleInformation(selected_sched - 1);
	else
	{
		if (shown_page - 1 < 0)
			return;
		showSchedule(shown_page - 1);
		selectScheduleInformation(max - 1, false);
	}

}

void osd::nextSchedulePage()
{
	if (shown_page + 1 >= numberSchedulePages())
		return;
	showSchedule(shown_page + 1);
	selectScheduleInformation(0, false);
}

void osd::prevSchedulePage()
{
	if (shown_page - 1 < 0)
		return;
	showSchedule(shown_page - 1);
	
	int max = 15;
	if ((shown_page + 1) * 15 > (int)sched.size())
		max = sched.size() % 15;

	selectScheduleInformation(max - 1, false);
}

int osd::getSelectedSchedule()
{
	if (sched.size() == 0)
		return 0;
	return sched[shown_page * 15 + selected_sched].eventid;
}

void osd::createIP()
{
	ip_position = 0;
	for (int i = 0; i < 12; i++)
	{
		ip[i] = 0;
	}
}

void osd::setIP(unsigned char number)
{
	ip[ip_position] = number;
	drawIPPosition(ip_position, 0);
}

void osd::drawIPPosition(int position, int color)
{
	fb.fillBox(90 + position * 40 + ((int)((float)position / 3)) * 10, 300, 125 + position * 40 + ((int)((float)position / 3)) * 10, 335, color);
	fb.putText(97 + position * 40 + ((int)((float)position / 3)) * 10, 325, color, ip[position]);
}

void osd::setIPPosition(unsigned char position)
{
	ip_position = position;

	drawIPPosition(ip_position, 0);
}

void osd::setIPNextPosition()
{
	drawIPPosition(ip_position, 5);
	ip_position++;
	if (ip_position > 11)
		ip_position = 0;

	setIPPosition(ip_position);
}

void osd::setIPPrevPosition()
{
	drawIPPosition(ip_position, 5);

	ip_position--;
	if (ip_position < 0)
		ip_position = 11;

	setIPPosition(ip_position);
}

void osd::setIPDescription(std::string descr)
{
	ip_description = descr;
}

void osd::showIP()
{
	for (int j = 0; j <= 10; j++)
	{
		fb.fillBox(150 + circlesmall[10 - j], 200 + j, 160, 200 + j + 1, 1);
		fb.fillBox(560, 200 + j, 570 - circlesmall[10 - j], 200 + j + 1, 1);
	}
	
	for (int j = 11; j < 20; j++)
	{
		fb.fillBox(150 + circlesmall[j - 11], 200 + j, 160, 200 + j + 1, 1);
		fb.fillBox(560, 200 + j, 570 - circlesmall[j - 11], 200 + j + 1, 1);
	}
	fb.fillBox(160, 200, 170, 220, 1);
	fb.fillBox(170, 200, 550, 220, 0);
	fb.fillBox(550, 200, 560, 220, 1);

	fb.setTextSize(0.4);
	fb.putText(175, 215, 0, ip_description, 370);

	fb.setTextSize(0.6);
	for (int j = 0; j <= 17; j++)
	{
		fb.fillBox(70 + circle[17 - j], 300 + j, 87, 300 + j + 1, 1);
		fb.fillBox(600, 300 + j, 618 - circle[17 - j], 300 + j + 1, 1);
	}
	for (int j = 18; j < 35; j++)
	{
		fb.fillBox(70 + circle[j - 18], 300 + j, 87, 300 + j + 1, 1);
		fb.fillBox(600, 300 + j, 618 - circle[j - 18], 300 + j + 1, 1);
	}

	for (int i = 0; i < 12; i++)
	{
		drawIPPosition(i, 5);
	}
}

void osd::hideIP()
{
	fb.clearScreen();
}

int osd::getIPPart(int number)
{
	return (ip[number * 3] * 100 + ip[number * 3 + 1] * 10 + ip[number * 3 + 2]);
}

void osd::showAbout()
{
	fb.fillBox(110, 100, 630, 420, 5);
	
	fb.setTextSize(1);

	fb.putText(240, 200, 5, setting.getVersion());

	fb.setTextSize(0.5);
	fb.putText(280, 250, 5, "GUI coded by TheDOC");
	fb.putText(190, 270, 5, "Drivers and stuff by the Tuxbox-Team");
	
	fb.setTextSize(0.4);
	fb.putText(195, 330, 5, "Info's about LCARS: http://www.chatville.de");
	fb.putText(200, 350, 5, "Info's about tuxbox: http://dbox2.elxsi.de");
}

void osd::hideAbout()
{
	fb.fillBox(110, 100, 630, 420, -1);
}
