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
/*
$Log: osd.h,v $
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef OSD_H
#define OSD_H

#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include "fbClass.h"
#include "settings.h"

typedef std::vector<std::string> command_list;

class osd
{
	struct list_entry
	{
		int index;
		std::string name;
	};



	list_entry list[10];
	int selected;
	int numberItems;
	settings setting;
	int circle[20];
	int circlesmall[20];
	int circlemiddle[20];
	
	fbClass *fb;
	
	// ProgramInfo
	std::string serviceName;
	int serviceNumber;
	bool redAvailable;
	bool greenAvailable;
	bool yellowAvailable;
	bool blueAvailable;
	time_t nowTime;
	time_t nextTime;
	std::string nowDescription;
	std::string nextDescription;
	char language[20];
	bool teletext;
	int par_rating;
	command_list prog_com_list_show;
	command_list prog_com_list_hide;
	command_list prog_com_list_servicenumber;
	command_list prog_com_list_servicename;
	command_list prog_com_list_language;
	command_list prog_com_list_nowtime;
	command_list prog_com_list_nowdescription;
	command_list prog_com_list_nexttime;
	command_list prog_com_list_nextdescription;
	
	// NumberEntry
	int number;
	std::string numberText;

	// perspective
	std::string perspective_name;

	// EPG
	std::string event_name;
	std::string event_short_text;
	std::string event_extended_text;
	std::string description;
	std::string programname;
	time_t starttime;
	int duration;
	int newlines[100];
	int nlcounter;
	int shown;

	// Menu
	struct menu_entry
	{
		int index;
		std::string caption;
		int type;
		int selected;
		std::vector<std::string> switches;
	};
	menu_entry menu[20];
	int selected_entry;
	int number_menu_entries;
	std::string menu_title;
	int menu_size;

	// Volume
	int volume;
	bool muted;

	// Scan
	int percentage;
	int channel_count;

	// Schedule
	struct scheduling
	{
		time_t starttime;
		std::string description;
		int eventid;
	};
	std::vector<struct scheduling> sched;
	int selected_sched;
	int shown_page;

	// IP
	unsigned char ip[12];
	short ip_position;
	std::string ip_description;
	std::queue<std::string> command_queue;
	
	pthread_t osdThread;
 
    static void* start_osdqueue( void * );
public:	
	bool proginfo_shown;
	time_t proginfo_hidetime;
	int start_thread();
	osd(settings &set, fbClass *fbclass);

	void initPalette();

	void loadSkin(std::string filename);

	void addCommand(std::string command);
	void executeCommand();
	void executeQueue();
	bool isEmpty() { return command_queue.empty(); }

	void clearScreen();

	void createList();
	int numberPossibleListItems();
	void addListItem(int index, std::string );
	void selectItem(int number);
	void selectNextItem();
	int selectedItem();
	void showList();
	void hideList();

	void createProgramInfo();
	void setServiceName(std::string name);
	void setLanguage(std::string language_name);
	void setServiceNumber(int number);
	void setRedAvailable(bool available);
	void setGreenAvailable(bool available);
	void setYellowAvailable(bool available);
	void setBlueAvailable(bool available);
	void setTeletext(bool available);
	void setNowTime(time_t starttime);
	void setNowDescription(std::string description);
	void setNextTime(time_t starttime);
	void setNextDescription(std::string description);
	void setParentalRating(int rating);
	void showProgramInfo();
	void hideProgramInfo();
	void setProgramCommandListShow(command_list list) { prog_com_list_show = list; }
	void setProgramCommandListHide(command_list list) { prog_com_list_hide = list; }
	void setProgramCommandListServiceNumber(command_list list) { prog_com_list_servicenumber = list; }
	void setProgramCommandListServiceName(command_list list) { prog_com_list_servicename = list; }
	void setProgramCommandListLanguage(command_list list) { prog_com_list_language = list; }
	void setProgramCommandListNowTime(command_list list) { prog_com_list_nowtime = list; }
	void setProgramCommandListNowDescription(command_list list) { prog_com_list_nowdescription = list; }
	void setProgramCommandListNextTime(command_list list) { prog_com_list_nexttime = list; }
	void setProgramCommandListNextDescription(command_list list) { prog_com_list_nextdescription = list; }
	

	void createEPG();
	void setEPGEventName(std::string input);
	void setEPGEventShortText(std::string input);
	void setEPGEventExtendedText(std::string input);
	void setEPGDescription(std::string input);
	void setEPGProgramName(std::string input);
	void setEPGstarttime(time_t input);
	void setEPGduration(int input);
	void showEPG();
	void showNextEPGPage();
	void showPrevEPGPage();
	void hideEPG();

	void createNumberEntry();
	void setNumberEntry(int setnumber);
	void setNumberText(std::string text);
	void showNumberEntry();
	void hideNumberEntry();

	void createPerspective();
	void setPerspectiveName(std::string name);
	void showPerspective();
	void hidePerspective();

	void createMenu();
	void setMenuSize(int size) { menu_size = size; }
	void addMenuEntry(int index, std::string caption, int type = 0);
	void addSwitchParameter(int number, std::string parameter);
	void setSelected(int number, int sel);
	int getSelected(int number) { return menu[number].selected; }
	void setMenuTitle(std::string title);
	void drawMenuEntry(int number, bool selected = false);
	int menuSelectedIndex();
	void selectEntry(int number);
	void selectNextEntry();
	void selectPrevEntry();
	void showMenu();
	void hideMenu();

	void setVolume(int vol) { volume = vol; }
	void setMute(bool mute);
	void showVolume();
	void hideVolume();

	void createScan();
	void setScanProgress(int percent);
	void setScanChannelNumber(int number);
	void showScan();
	void hideScan();

	void createSchedule();
	void addScheduleInformation(time_t starttime, std::string description, int eventid);
	int numberSchedulePages();
	int getShownPage() { return shown_page; }
	void selectNextScheduleInformation();
	void selectPrevScheduleInformation();
	void nextSchedulePage();
	void prevSchedulePage();
	int getSelectedSchedule();
	void selectScheduleInformation(int select, bool redraw = true);
	void showSchedule(int page);
	void hideSchedule();

	void createIP();
	void setIP(unsigned char number);
	void drawIPPosition(int position, int color);
	void setIPDescription(std::string descr);
	void setIPPosition(unsigned char position);
	void setIPNextPosition();
	void setIPPrevPosition();
	int getIPPart(int number);
	void showIP();
	void hideIP();

	void showAbout();
	void hideAbout();
};

#endif
