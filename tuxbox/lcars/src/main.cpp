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

#include <iostream.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dbox/info.h>
#include <dbox/gen_vbi.h>
#include <string>
#include <stdio.h>


#include "sdt.h"
#include "zap.h"
#include "nit.h"
#include "pat.h"
#include "pmt.h"
#include "eit.h"
#include "settings.h"
#include "rc.h"
#include "channels.h"
#include "osd.h"
#include "checker.h"
#include "container.h"
#include "serial.h"
#include "teletext.h"
#include "network.h"
#include "scan.h"
#include "hardware.h"
#include "tot.h"
#include "plugins.h"
#include "pig.h"
#include "timer.h"

// del
#include <stdio.h>
// del

int main(int argc, char **argv)
{
	int key = -1;
	int number = -1;	
	std::string font = "/usr/lib/fonts/ds9.ttf";

	plugins plugins;
	
	
	FILE *fp;
	fp = fopen("/var/lcars/dirs.conf", "r");
	
	if (fp < 0)
	{
		printf("File /var/lcars/dirs.conf not found!");
		exit(0);
	}
	char pluginpath[100];
	fgets(pluginpath, 100, fp);
	printf("Plugins-Pfad: %s\n", pluginpath);
	fclose(fp);
	
	std::string ppath(pluginpath);
		
	plugins.setPluginDir(ppath.substr(0, ppath.length() - 1));
	plugins.loadPlugins();
	
	cam cam;
	sdt sdt;
	nit nit;
	cam.readCAID();	

	settings settings(cam);

	settings.setVersion("LCARS V0.13");

	hardware hardware(settings);

	rc rc(&hardware);
	rc.start_thread();
	
	char n0 = settings.getIP(0);
	char n1 = settings.getIP(1);
	char n2 = settings.getIP(2);
	char n3 = settings.getIP(3);

	if (!((n0 == 0) && (n1 == 0) && (n2 == 0) && (n3 == 0)))
	{
		char command[100];
		sprintf(command, "ifconfig eth0 %d.%d.%d.%d", n0, n1, n2, n3);
		printf("%s\n", command);
		system(command);
	}


	if (argc == 3 || argc == 4 || argc == 5)
		rc.setRepeat(strcmp(argv[2], "repeat=off"));
	else
		rc.setRepeat(true);
	
	if (argc == 4 || argc == 5)
	{
		rc.setSupportOld(!strcmp(argv[3], "old=on"));
	}

	if (argc == 5)
	{
		font = argv[4];
	}
	printf("Starting OSD\n");
	fbClass fb(settings);
	osd osd(settings, fb, font);
	osd.start_thread();
	
	pig pig;
	pig.hide();

	printf("Ending OSD\n");


	int test = open("/dev/ost/demux0", O_RDWR);
	if (test < 0)
	{
		osd.addCommand("CREATE ip");
		osd.setIPDescription("Please enter IP-address!");
		osd.addCommand("SHOW ip");
		osd.addCommand("COMMAND ip position 0");
		do
		{
			key = rc.read_from_rc();
			number = rc.get_number();
			if (number != -1)
			{
				osd.setIP(number);
				osd.setIPNextPosition();
			}
			else if (key == RC1_RIGHT)
			{
				osd.setIPNextPosition();
			}
			else if (key == RC1_LEFT)
			{
				osd.setIPPrevPosition();
			}
		} while ( key != RC1_OK && key != RC1_HOME);
		if (key == RC1_OK)
		{
			settings.setIP(osd.getIPPart(0), osd.getIPPart(1), osd.getIPPart(2), osd.getIPPart(3));

			char command[100];
			sprintf(command, "ifconfig eth0 %d.%d.%d.%d", osd.getIPPart(0), osd.getIPPart(1), osd.getIPPart(2), osd.getIPPart(3));
			printf("%s\n", command);
			system(command);
				
			system(command);
		}
		osd.addCommand("HIDE ip");
		osd.createPerspective();
		
		osd.setPerspectiveName("Please copy the ucodes now!");
		osd.addCommand("SHOW perspective");
		sleep(4);
		exit(1);
	}
	close(test);

	printf("Starting\n");
	tuner tuner(settings);
	zap zap(settings, osd, tuner, cam);
	
	pmt pmt;
	pat pat;
	tot tot(&settings);

	eit eit(&settings, &osd);
	eit.start_thread();


	channels channels(settings, pat, pmt, &eit);
	checker checker;
	

	checker.start_16_9_thread();
	
	scan scan(settings, pat, pmt, nit, sdt, &osd, tuner);

	if (rc.command_available())
	{
		int com = rc.read_from_rc();
		if (com == RC1_HELP)
		{
			printf("Emergency channel-scan\n");
			channels = scan.scanChannels();
			channels.saveDVBChannels();
			while(rc.command_available())
				rc.read_from_rc();
		}

	}

	
	channels.loadDVBChannels();

	if (channels.numberChannels() == 0)
	{
		channels = scan.scanChannels();
		channels.saveDVBChannels();
	}
	
	container container(&zap, &channels, &fb, &osd, &settings, &tuner, &pat, &pmt, &eit, &scan);
	
	channels.setBeginTS();
	while (channels.getCurrentFrequency() == 0)
		channels.setNextTS();

	channels.tuneCurrentTS(&tuner);

	settings.getEMMpid();
	
	printf("container-chans: %d\n", (*container.channels_obj).numberChannels());

	network network(container);
	network.startThread();

	timer timer(&hardware, &channels, &zap, &tuner, &osd);
	timer.loadTimer();
	timer.start_thread();
	tot.start_thread();
	
	int mode = 0; // 0 = Main Menu

	int final_number;
	bool finish;

	int channelnumber;
	int old_channel;
	if (argc == 1)
	{
		channelnumber = 0;
		old_channel = 0;
	}
	else
	{
		channelnumber = atoi(argv[1]);
		old_channel = atoi(argv[1]);
	}

	int apid = 0;
	linkage perspective[10];
	int number_perspectives = 0;
	int position;
	int selected;
	int curr_perspective = 0;
	int old_TS = 0;
	pmt_data pmt_entry;
	int ECM = 0;
	event now, next, nowref;
	int component[10];
	char audio_description[20];
	int curr_nvod = 0, nvod_count = 0;
	channel nvods[10];
	int APIDcount = 0;
	int video_component = 0;
	int switchmode = 2;
	int currentepg = 1;
	bool change = true;
	int positionepg = 0;
	bool allowkeys = true;
	bool leave = false;
	bool schedule_read = false;
	char text[20];
	int txtfd;

	do
	{
		
		time_t act_time;

		switch (mode)
		{
		case 0:
			if (channelnumber > channels.numberChannels())
				channelnumber = 0;
			channels.setCurrentChannel(channelnumber);

			if (channels.getCurrentType() > 4 || channels.getCurrentType() < 1)
			{
				channelnumber++;
				continue;
			}
			
			osd.addCommand("SHOW proginfo 5");
			channels.setCurrentOSDProgramInfo(&osd);
			channels.zapCurrentChannel(&zap, &tuner);
			
			schedule_read = false;
			if (channels.getCurrentTXT() != 0)
			{
				osd.addCommand("COMMAND proginfo set_teletext true");
				
				txtfd = open("/dev/dbox/vbi0", O_RDWR);
				ioctl(txtfd, VBI_START_VTXT, channels.getCurrentTXT());

				close(txtfd);
			}
			else
				osd.addCommand("COMMAND proginfo set_teletext false");

			printf("Channel: %s\n", channels.getCurrentServiceName().c_str());
			printf("SID: %04x\n",channels.getCurrentSID());
			printf("PMT: %04x\n", pat.getPMT(channels.getCurrentSID()));

			printf("Audio-PIDs: %d\n", channels.getCurrentAPIDcount());
			
			
			channels.receiveCurrentEIT();
			
			mode = 2;
			break;

		case 1: // Wait for key or timeout
			printf("Warten auf timeout...\n");
			act_time = time(0);
			while ((!rc.command_available()) && (time(0) - act_time < 5));
			mode = switchmode;
			break;
		case 2: // Main Key-Loop
			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DBOX)
					{
						hardware.switch_vcr();
						allowkeys = !allowkeys;
					}
					if (key == RC1_RIGHT)
					{
						apid++;
						if (apid >= channels.getCurrentAPIDcount())
							apid = 0;
						channels.zapCurrentAudio(apid, &zap);
					}
					else if (key == RC1_LEFT)
					{
						apid--;
						if (apid < 0 )
							apid = channels.getCurrentAPIDcount() - 1;
						
						channels.zapCurrentAudio(apid, &zap);
					}
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}

				} while(!allowkeys);
			} while (key != RC1_VOLPLUS && key != RC1_VOLMINUS && key != RC1_UP && key != RC1_DOWN && key != RC1_STANDBY && number == -1 && key != RC1_OK && key != RC1_RED && key != RC1_GREEN && key != RC1_BLUE && key != RC1_YELLOW && key != RC1_HELP);
			
			channelnumber = channels.getCurrentChannelNumber();
			if (key == RC1_DOWN)
			{
				old_channel = channelnumber;
				while (channels.getType(++channelnumber) > 4);
				if (channelnumber >= channels.numberChannels())
					channelnumber = 0;
				apid = 0;
			}
			else if (key == RC1_0)
			{
				int tmp = old_channel;
				old_channel = channelnumber;
				channelnumber = tmp;
				apid = 0;
			}
			else if (key == RC1_UP)
			{
				old_channel = channelnumber;
				while (channels.getType(--channelnumber) > 4);
				if (channelnumber < 0 )
					channelnumber = channels.numberChannels() - 1;
				apid = 0;
			}
			else if (key == RC1_VOLPLUS)
			{
				osd.setVolume(63 - hardware.vol_plus(5));
				osd.showVolume();
				mode = 11;
				switchmode = 2;
				continue;	
			}
			else if (key == RC1_VOLMINUS)
			{
				osd.setVolume(63 - hardware.vol_minus(5));
				osd.showVolume();
				mode = 11;
				switchmode = 2;
				continue;
			}
			else if (key == RC1_HELP)
			{
				if (!schedule_read)
				{
					eit.dumpSchedule(channels.getCurrentSID(), &osd);
					schedule_read = true;
				}

				osd.createSchedule();
				eit.dumpSchedule(channels.getCurrentTS(), channels.getCurrentONID(), channels.getCurrentSID(), &osd);
				printf("Wow\n");
				osd.showSchedule(0);
				osd.selectScheduleInformation(0);
				mode = 12;
				switchmode = 2;
				continue;
			}

			mode = 0;
			if (number >= 1)
				mode = 3;
			if (key == RC1_OK)
			{
				mode = 2;
				if (osd.proginfo_shown)
					osd.addCommand("HIDE proginfo");
				else
					osd.addCommand("SHOW proginfo 5");
			}
			else if (key == RC1_RED)
			{
				mode = 4;
			}
			else if (key == RC1_YELLOW)
			{
				osd.createEPG();
				now = eit.getNow();
				next = eit.getNext();
				osd.setEPGEventName(now.event_name);
				osd.setEPGEventShortText(now.event_short_text);
				osd.setEPGEventExtendedText(now.event_extended_text);
				osd.setEPGProgramName(channels.getCurrentServiceName());
				osd.setEPGstarttime(now.starttime);
				osd.setEPGduration(now.duration);
				change = true;
				currentepg = 1;
				positionepg = 0;
				mode = 8;
			}
			else if (key == RC1_BLUE)
			{
					mode = 9;
					switchmode = 2;
			}
			else if ((eit.isMultiPerspective()) && (key == RC1_GREEN))
			{	
				mode = 5;
				number_perspectives = eit.numberPerspectives();
				for (int i = 0; i < number_perspectives; i++)
				{
					perspective[i] = eit.nextLinkage();
					printf("%s\n", perspective[i].name);
				}
				curr_perspective = 0;
				osd.createPerspective();
				char message[100];
				sprintf(message, "Please choose perspective (%d - %d)", 1, number_perspectives);
				osd.setPerspectiveName(message);
				osd.addCommand("SHOW perspective");
 			}
			else if ((channels.getCurrentType() == 4) && (key == RC1_GREEN))
			{
				mode = 7;
				curr_nvod = 0;
				osd.createPerspective();
				osd.setPerspectiveName("Reading NVOD-Data...");
				osd.addCommand("SHOW perspective");
				sdt.getNVODs(&channels);
				nvod_count = channels.getCurrentNVODcount();
				{
					
					for (int i = 0; i < nvod_count; i++)
					{
						printf("NVOD: TS: %x - SID: %x\n", channels.getCurrentNVOD_TS(i),channels.getCurrentNVOD_SID(i));
						nvods[i].TS = channels.getCurrentNVOD_TS(i);
						nvods[i].SID = channels.getCurrentNVOD_SID(i);
					}

				}
				
				char message[100];
				sprintf(message, "Please choose nvod (%d - %d)", 1, nvod_count);
				osd.setPerspectiveName(message);
				osd.addCommand("SHOW perspective");
				continue;
			}
			break;
		case 3: // Number entry
			final_number = number;
			finish = false;
			osd.createNumberEntry();
			osd.setNumberEntry(number);
			osd.setNumberText(channels.getServiceName(number));
			osd.addCommand("SHOW number");
			printf("%d\n", final_number);
			do
			{
				act_time = time(0);
				while ((!rc.command_available()) && (time(0) - act_time < 2));
				printf("%d\n", (int) (time(0) - act_time));
				if (time(0) - act_time >= 2)
					finish = true;
				else if (rc.command_available())
				{
					key = rc.read_from_rc();
					int tmp_number = rc.get_number();
					if (tmp_number != -1)
					{
						final_number = final_number * 10 + tmp_number;
						osd.setNumberEntry(final_number);
						osd.setNumberText(channels.getServiceName(final_number));
						osd.addCommand("SHOW number");
					}
					else if (key == RC1_OK)
						finish = true;
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				}
			} while (!finish);
			old_channel = channelnumber;
			channelnumber = final_number;
			osd.addCommand("HIDE number");
			mode = 0;
			break;
		case 4: // Channel-List
			position = (int) (channelnumber / 10);
			selected = channelnumber % 10;
			osd.createList();
			for (int i = position * 10; i < position * 10 + 10; i++)
			{
				osd.addListItem(i, channels.getServiceName(i));
			}
			osd.addCommand("SHOW list");
			char cmd_text[100];
			sprintf(cmd_text, "COMMAND list select_item %d", selected);
			osd.addCommand(cmd_text);
			do
			{
				key = rc.read_from_rc();
				number = rc.get_number();
				if (key == RC1_DBOX)
					hardware.switch_vcr();
				if (key == RC1_UP)
				{
					selected--;
					if (selected < 0)
					{
						position--;
						if (position < 0)
							position = (int) (channels.numberChannels() / 10);
						selected = 9;
						osd.createList();
						for (int i = position * 10; i < position * 10 + 10; i++)
						{
							osd.addListItem(i, channels.getServiceName(i));//.substr(0,8));
						}
						osd.addCommand("SHOW list");
					}

					char cmd_text[100];
					sprintf(cmd_text, "COMMAND list select_item %d", selected);
					osd.addCommand(cmd_text);
				}
				if (key == RC1_DOWN)
				{
					selected++;
					if (selected > 9)
					{
						position++;
						if (position > (int)(channels.numberChannels() / 10))
							position = 0;
						selected = 0;
						osd.createList();
						for (int i = position * 10; i < position * 10 + 10; i++)
						{
							osd.addListItem(i, channels.getServiceName(i));//.substr(0,8));
						}
						osd.addCommand("SHOW list");
					}

					char cmd_text[100];
					sprintf(cmd_text, "COMMAND list select_item %d", selected);
					osd.addCommand(cmd_text);
				}
				if (key == RC1_RIGHT)
				{
					position++;
					if (position > (int)(channels.numberChannels() / 10))
						position = 0;
					selected = 0;
					osd.createList();
					for (int i = position * 10; i < position * 10 + 10; i++)
					{
						osd.addListItem(i, channels.getServiceName(i));//.substr(0,8));
					}
					osd.addCommand("SHOW list");
					char cmd_text[100];
					sprintf(cmd_text, "COMMAND list select_item %d", selected);
					osd.addCommand(cmd_text);
				}
				if (key == RC1_LEFT)
				{
					position--;
					if (position < 0)
						position = (int) (channels.numberChannels() / 10);
					selected = 9;
					osd.createList();
					for (int i = position * 10; i < position * 10 + 10; i++)
					{
						osd.addListItem(i, channels.getServiceName(i));//.substr(0,8));
					}
					osd.addCommand("SHOW list");
					char cmd_text[100];
					sprintf(cmd_text, "COMMAND list select_item %d", selected);
					osd.addCommand(cmd_text);
				}
				if (number != -1)
				{
					if (number != 0)
						osd.selectItem(number - 1);
					else
						osd.selectItem(9);
					selected = osd.selectedItem();
				}
				else if (key == RC1_MUTE)
				{
					if (hardware.isMuted())
					{
						osd.setMute(false);
						hardware.switch_mute();
					}
					else
					{
						osd.setMute(true);
						hardware.switch_mute();
					}
				}
			} while (key != RC1_OK && key != RC1_STANDBY && key != RC1_RED && key != RC1_HOME);
			if (key == RC1_RIGHT)
			{
				apid++;
				if (apid >= channels.getCurrentAPIDcount())
					apid = 0;
			}
			osd.addCommand("HIDE list");
			mode = 0;
			if (key == RC1_RED || key == RC1_HOME)
			{
				mode = 2;
			}
			else if (key == RC1_OK)
			{
				old_channel = channelnumber;
				channelnumber = position * 10 + osd.selectedItem();
			}
			break;
		case 5: // linkage
			do
			{
				key = rc.read_from_rc();
				number = rc.get_number();
				if (key == RC1_YELLOW)
					osd.clearScreen();
				else if (key == RC1_MUTE)
				{
					if (hardware.isMuted())
					{
						osd.setMute(false);
						hardware.switch_mute();
					}
					else
					{
						osd.setMute(true);
						hardware.switch_mute();
					}
					
				}
				else if (key == RC1_RIGHT)
				{
					apid++;
					if (apid >= perspective[curr_perspective].APIDcount)
						apid = 0;
	
					zap.zap_audio(perspective[curr_perspective].VPID, perspective[curr_perspective].APID[apid] , ECM, perspective[curr_perspective].SID, perspective[curr_perspective].ONID);
						
					for (int i = 0; i < now.number_components; i++)
					{
						if (now.component_tag[i] == component[apid])
						{
							strcpy(audio_description, now.audio_description[i]);
						}
					}
				}
				else if (key == RC1_LEFT)
				{
					apid--;
					if (apid < 0 )
						apid = perspective[curr_perspective].APIDcount - 1;

					zap.zap_audio(perspective[curr_perspective].VPID, perspective[curr_perspective].APID[apid] , ECM, perspective[curr_perspective].SID, perspective[curr_perspective].ONID);
							
					for (int i = 0; i < now.number_components; i++)
					{
						if (now.component_tag[i] == component[apid])
						{
							strcpy(audio_description, now.audio_description[i]);
						}
					}
				}
			} while(key != RC1_GREEN && number == -1 && key != RC1_BLUE && key != RC1_STANDBY && key != RC1_DOWN && key != RC1_UP);

			if (key == RC1_GREEN)
			{
				mode = 0;
				continue;
			}
			else
			{
				
				if (number != -1)
				{
					
					if (number < number_perspectives + 1 && number > 0)
						curr_perspective = (number % (eit.numberPerspectives() + 1)) - 1;

				}
				mode = 6;
				
			}
			if (key == RC1_UP)
			{
				curr_perspective++;
				if (curr_perspective >= number_perspectives)
					curr_perspective = 0;
				mode = 6;
			}
			else if (key == RC1_DOWN)
			{
				curr_perspective--;
				if (curr_perspective <0)
					curr_perspective = number_perspectives - 1;
				mode = 6;
			}
			else if (key == RC1_BLUE)
			{
				mode = 9;
				switchmode = 5;
				continue;
			}
			printf("----------------------\n");
			printf("APID: %d\n", apid);
			printf("Current perspective: %d\n", curr_perspective);
			if (old_TS != perspective[curr_perspective].TS)
				channels.tune(perspective[curr_perspective].TS, &tuner);
			old_TS = perspective[curr_perspective].TS;
			
			zap.close_dev();
			pat.readPAT();
			ECM = 0;
			
			memset (&pmt_entry, 0, sizeof (struct pmt_data));
			pmt_entry = pmt.readPMT(pat.getPMT(perspective[curr_perspective].SID));
			channels.deleteCurrentAPIDs();
			perspective[curr_perspective].APIDcount = 0;
			for (int i = 0; i < pmt_entry.pid_counter; i++)
			{
				if (pmt_entry.type[i] == 0x02)
					perspective[curr_perspective].VPID = pmt_entry.PID[i];
				else if (pmt_entry.type[i] == 0x04 || pmt_entry.type[i] == 0x03)
				{
					printf("an APID: %04x\n", pmt_entry.PID[i]);
					perspective[curr_perspective].APID[perspective[curr_perspective].APIDcount++] = pmt_entry.PID[i];
				}
				printf("type: %d - PID: %04x\n", pmt_entry.type[i], pmt_entry.PID[i]);
			}

			printf("ECMs: %d\n", pmt_entry.ecm_counter);
				
			for (int i = 0; i < pmt_entry.ecm_counter; i++)
			{
				if (settings.getCAID() == pmt_entry.CAID[i])
					ECM = pmt_entry.ECM[i];
				printf("CAID: %04x - ECM: %04x\n", pmt_entry.CAID[i], pmt_entry.ECM[i]);

			}
			osd.addCommand("HIDE perspective");
			osd.createPerspective();
			osd.setPerspectiveName(perspective[curr_perspective].name);
			osd.addCommand("SHOW perspective");
			printf("%s\n", perspective[curr_perspective].name);
			if (perspective[curr_perspective].APIDcount == 1)
				zap.zap_to(perspective[curr_perspective].VPID, perspective[curr_perspective].APID[apid] , ECM, perspective[curr_perspective].SID, perspective[curr_perspective].ONID, perspective[curr_perspective].TS);
			else
				zap.zap_to(perspective[curr_perspective].VPID, perspective[curr_perspective].APID[0], ECM, perspective[curr_perspective].SID, perspective[curr_perspective].ONID, perspective[curr_perspective].TS, perspective[curr_perspective].APID[1]);

			schedule_read = false;
			break;
			
		case 6:
			printf("Warten auf timeout...\n");
			act_time = time(0);
			while ((!rc.command_available()) && (time(0) - act_time < 3));
			osd.addCommand("HIDE perspective");
			mode = 5;
			break;
		case 7: // NVOD
			printf("NVOD\n");

			do
			{
				key = rc.read_from_rc();
				number = rc.get_number();
				if (key == RC1_YELLOW)
					osd.clearScreen();
				else if (key == RC1_MUTE)
				{
					if (hardware.isMuted())
					{
						osd.setMute(false);
						hardware.switch_mute();
					}
					else
					{
						osd.setMute(true);
						hardware.switch_mute();
					}
				}
				else if (key == RC1_RIGHT)
				{
					apid++;
					if (apid >= APIDcount)
						apid = 0;

					zap.zap_audio(nvods[curr_nvod].VPID, nvods[curr_nvod].APID[apid] , ECM, nvods[curr_nvod].SID, 0x85);
						
					for (int i = 0; i < now.number_components; i++)
					{
						if (now.component_tag[i] == component[apid])
						{
							strcpy(audio_description, now.audio_description[i]);
						}
					}
	
					if (channels.getCurrentAPIDcount() > 1)
							osd.setLanguage(audio_description);
				}
				else if (key == RC1_LEFT)
				{
					apid--;
					if (apid < 0 )
						apid = APIDcount - 1;

					zap.zap_audio(nvods[curr_nvod].VPID, nvods[curr_nvod].APID[apid] , ECM, nvods[curr_nvod].SID, 0x85);
						
					for (int i = 0; i < now.number_components; i++)
					{
						if (now.component_tag[i] == component[apid])
						{
							strcpy(audio_description, now.audio_description[i]);
						}
					}
	
					if (channels.getCurrentAPIDcount() > 1)
							osd.setLanguage(audio_description);
				}
			
			} while(key != RC1_GREEN && number == -1 && key != RC1_STANDBY && key != RC1_BLUE && key != RC1_OK);
			osd.addCommand("HIDE perspective");
			printf("%d\n", number);
			if (key == RC1_GREEN)
			{
				mode = 0;
				continue;
			}
			if (key == RC1_BLUE)
			{
				mode = 9;
				switchmode = 7;
				continue;
			}
			else if (key == RC1_OK)
			{
				switchmode = 7;
				mode = 1;
				if (next.starttime <= time(0))
				{
					eit.receiveNow(nvods[curr_nvod].SID);
				}
				if (osd.proginfo_shown)
					osd.addCommand("HIDE proginfo");
				else
					osd.addCommand("SHOW proginfo 5");
				break;
			}
			else
			{
				
				if (number != -1)
				{
					
					if (number < nvod_count + 1)
						curr_nvod = (number % (nvod_count + 1)) - 1;

				}				
			}

			printf("----------------------\n");
			printf("APID: %d\n", apid);
			printf("Current NVOD: %d\n", curr_nvod);
			if (old_TS != nvods[curr_nvod].TS)
				channels.tune(nvods[curr_nvod].TS, &tuner);
			
			printf("Tuning to TS: %d\n", nvods[curr_nvod].TS);
			
			
			zap.close_dev();
			
			old_TS = nvods[curr_nvod].TS;

			pat.readPAT();
			
			ECM = 0;
			
			memset (&pmt_entry, 0, sizeof (struct pmt_data));
			if (pat.getPMT(nvods[curr_nvod].SID) != 0)
				pmt_entry = pmt.readPMT(pat.getPMT(nvods[curr_nvod].SID));

			APIDcount = 0;	
			for (int i = 0; i < pmt_entry.pid_counter; i++)
			{
				if (pmt_entry.type[i] == 0x02)
					nvods[curr_nvod].VPID = pmt_entry.PID[i];
				else if (pmt_entry.type[i] == 0x04 || pmt_entry.type[i] == 0x03)
				{
					printf("an APID: %04x\n", pmt_entry.PID[i]);
					nvods[curr_nvod].APID[APIDcount++] = pmt_entry.PID[i];
				}
				printf("type: %d - PID: %04x\n", pmt_entry.type[i], pmt_entry.PID[i]);
			}

			printf("ECMs: %d\n", pmt_entry.ecm_counter);
				
			for (int i = 0; i < pmt_entry.ecm_counter; i++)
			{
				if (settings.getCAID() == pmt_entry.CAID[i])
					ECM = pmt_entry.ECM[i];
				printf("CAID: %04x - ECM: %04x\n", pmt_entry.CAID[i], pmt_entry.ECM[i]);

			}
			
			if (APIDcount == 1)
				zap.zap_to(nvods[curr_nvod].VPID, nvods[curr_nvod].APID[apid] , ECM, nvods[curr_nvod].SID, channels.getCurrentONID(), nvods[curr_nvod].TS);
			else
				zap.zap_to(nvods[curr_nvod].VPID, nvods[curr_nvod].APID[0] , ECM, nvods[curr_nvod].SID, channels.getCurrentONID(), nvods[curr_nvod].TS, nvods[curr_nvod].APID[1]);
			
			schedule_read = false;

			eit.receiveNow(nvods[curr_nvod].SID);
			now = eit.getNow();
			next = eit.getNext();

			strcpy(audio_description, "");
			
			for (int i = 0; i < nowref.number_components; i++)
			{
				printf("Component_tag: %x\n", nowref.component_tag[i]);
				if (nowref.component_tag[i] == video_component)
				{
					printf("Video_component_type: %d\n", nowref.component_type[i]);
				}
				else if (nowref.component_tag[i] == component[apid])
				{
					strcpy(audio_description, nowref.audio_description[i]);
				}
			}
			
			if (channels.getCurrentAPIDcount() > 1)
				osd.setLanguage(audio_description);

			osd.setNowTime(now.starttime);
			osd.setNextTime(next.starttime);
			osd.addCommand("SHOW proginfo 5");
			switchmode = 7;
			mode = 1;

			break;

		case 8:
			mode = 2;
			if (change)
			{
				osd.addCommand("SHOW epg");
				change = false;
			}
			
			do
			{
				key = rc.read_from_rc();
				if (key == RC1_MUTE)
				{
					if (hardware.isMuted())
					{
						osd.setMute(false);
						hardware.switch_mute();
					}
					else
					{
						osd.setMute(true);
						hardware.switch_mute();
					}
				}
				else if (key == RC1_DOWN)
				{
					osd.addCommand("SHOW epg_next_page");
				}
				else if (key == RC1_UP)
				{
					osd.addCommand("SHOW epg_prev_page");
				}
				number = rc.get_number();
			} while(key != RC1_RIGHT && key != RC1_LEFT && key != RC1_YELLOW && key != RC1_HOME && key != RC1_OK && key != RC1_STANDBY);
			if (key == RC1_RIGHT)
			{
				if (currentepg != 2)
				{
					osd.setEPGEventName(next.event_name);
					osd.setEPGEventShortText(next.event_short_text);
					osd.setEPGEventExtendedText(next.event_extended_text);
					osd.setEPGstarttime(next.starttime);
					osd.setEPGduration(next.duration);
					currentepg = 2;
					change = true;
					positionepg = 0;
				}
				mode = 8;
			}
			else if (key == RC1_LEFT)
			{
				if (currentepg != 1)
				{
					osd.setEPGEventName(now.event_name);
					osd.setEPGEventShortText(now.event_short_text);
					osd.setEPGEventExtendedText(now.event_extended_text);
					osd.setEPGstarttime(now.starttime);
					osd.setEPGduration(now.duration);
					currentepg = 1;
					change = true;
					positionepg = 0;
				}
				mode = 8;
			}

			else 
				osd.addCommand("HIDE epg");
			break;
		case 9: // Main menu
			osd.createMenu();
			osd.setMenuTitle("Main Menu");

			osd.addMenuEntry(1, "About");

			osd.addMenuEntry(2, "Timer");

			osd.addMenuEntry(3, "Recording");


			osd.addMenuEntry(4, "PID");
			
			if (channels.getCurrentTXT() != 0)
				osd.addMenuEntry(5, "Teletext");
			
			osd.addMenuEntry(6, "Plug-Ins");
			
			osd.addMenuEntry(0, "Setup", 3);

			osd.addMenuEntry(8, "Visual Setup");

			osd.addMenuEntry(9, "General Setup");
			osd.addCommand("SHOW menu");

			osd.addCommand("COMMAND menu select next");
			
		
			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DOWN)
					{
						osd.selectNextEntry();
					}
					if (key == RC1_UP)
					{
						osd.selectPrevEntry();
					}
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				} while(key != RC1_OK && key != RC1_HOME && key != RC1_BLUE && key != RC1_LEFT && key != RC1_RIGHT && number == -1);
				
				if (key == RC1_OK)
				{
					number = osd.menuSelectedIndex();
				}
				
				leave = true;
				if (number == 1)
				{
					osd.addCommand("HIDE menu");
					osd.addCommand("SHOW about");
					rc.read_from_rc();
					osd.addCommand("HIDE about");
					continue;
				}
				else if (number == 2)
				{
					mode = 17;
				}
				else if (number == 3)
				{
					zap.dmx_stop();
					mode = 15;
				}
				else if (number == 4)
				{
					mode = 15;
				}
				else if (number == 5)
				{
					printf("Teletext\n");
					if (channels.getCurrentTXT() != 0)
					{
					//	teletext.getTXT(channels.getCurrentTXT());
					}
				}
				else if (number == 6)
				{
					mode = 16;
				}
				else if (number == 9)
				{
					mode = 10;
				}
				else if (key == RC1_HOME || key == RC1_BLUE || key == RC1_LEFT || key == RC1_RIGHT)
				{
					mode = switchmode;
				}
				else
					leave = false;
			} while (!leave);
			osd.addCommand("HIDE menu");
			
			break;
		case 10: // General Setup
			osd.createMenu();
			osd.setMenuTitle("General Setup");

			osd.addMenuEntry(1, "16:9 Format", 2);
			osd.addSwitchParameter(0, "Letterbox"); // 2
			osd.addSwitchParameter(0, "Panscan"); // 1
			osd.addSwitchParameter(0, "Centercut"); // 0
			osd.setSelected(0, checker.get_16_9_mode()); // Centercut

			osd.addMenuEntry(2, "RC Repeat", 1);
			osd.setSelected(1, rc.getRepeat());

			osd.addMenuEntry(3, "Scart", 2);
			osd.addSwitchParameter(2, "FBAS"); // 1
			osd.addSwitchParameter(2, "RGB"); // 0
			if (hardware.getfblk() == 0)
				osd.setSelected(0, 0);
			else 
				osd.setSelected(0, 1);

			osd.addMenuEntry(4, "Support old RC", 1);
			osd.setSelected(1, rc.getSupportOld());

			osd.addMenuEntry(0, "Scan-Options", 3);

			osd.addMenuEntry(6, "Update-Channel-Scan");

			osd.addMenuEntry(7, "FULL Channel-Scan");

			osd.addMenuEntry(8, "Channel-Scan");
			
			osd.addMenuEntry(0, "Setup-Stuff", 3);
			
			osd.addMenuEntry(9, "Box-Setup");

			osd.addCommand("SHOW menu");

			osd.addCommand("COMMAND menu select next");
			
			leave = false;
			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DOWN)
					{
						osd.selectNextEntry();
					}
					if (key == RC1_UP)
					{
						osd.selectPrevEntry();
					}
					if (key == RC1_OK)
						number = osd.menuSelectedIndex();
					if (number == 1)
					{
						if (osd.getSelected(0) == 2)
							osd.setSelected(0, 0);
						else
							osd.setSelected(0, osd.getSelected(0) + 1);
						checker.set_16_9_mode(osd.getSelected(0));
						
						osd.selectEntry(0);
					}
					else if (number == 2)
					{
						osd.setSelected(1, !osd.getSelected(1));
						rc.setRepeat(osd.getSelected(1));
						osd.selectEntry(1);
					}
					else if (number == 3)
					{
						if (osd.getSelected(2) == 1)
							osd.setSelected(2, 0);
						else
							osd.setSelected(2, osd.getSelected(2) + 1);
						
						if (osd.getSelected(2) == 0)
							hardware.setfblk(0);
						else if (osd.getSelected(2) == 1)
							hardware.setfblk(3);
						
						
						osd.selectEntry(2);
					}
					else if (number == 4)
					{
						osd.setSelected(3, !osd.getSelected(3));
						rc.setSupportOld(osd.getSelected(3));
						if (osd.getSelected(3))
						{
							rc.setRepeat(true);
							osd.setSelected(1, true);
							osd.drawMenuEntry(1);
						}
						osd.selectEntry(3);
					}
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				} while(key != RC1_OK && key != RC1_HOME && key != RC1_RIGHT && key != RC1_LEFT && key != RC1_BLUE && number == -1);
				
				if (key == RC1_OK)
				{
					number = osd.menuSelectedIndex();
				}
				
				leave = true;

				if (key == RC1_HOME || key == RC1_BLUE)
				{
					mode = switchmode;
				}
				else if (key == RC1_RIGHT || key == RC1_LEFT)
				{
					mode = 9;
				}
				else if (number == 6)
				{
					osd.addCommand("HIDE menu");
					scan.updateChannels(&channels);
					channels.saveDVBChannels();
					osd.addCommand("SHOW menu");


				}
				else if (number == 7)
				{
					osd.addCommand("HIDE menu");
					channels = scan.scanChannels(true);
					channels.saveDVBChannels();
					osd.addCommand("SHOW menu");
				}
				else if (number == 8)
				{
					osd.addCommand("HIDE menu");
					channels = scan.scanChannels();
					channels.saveDVBChannels();
					osd.addCommand("SHOW menu");
				}
				else if (number == 9)
				{
					osd.addCommand("HIDE menu");
					mode = 13;
				}
				else
					leave = false;
			} while (!leave);
			osd.addCommand("HIDE menu");
			
			break;
		case 11: // Wait for key or timeout
			printf("Warten auf timeout...\n");
			act_time = time(0);
			while ((!rc.command_available()) && (time(0) - act_time < 3));
			osd.hideVolume();
			mode = switchmode;
			break;
		case 12:
			do
			{
				key = rc.read_from_rc();
				if (key == RC1_MUTE)
				{
					if (hardware.isMuted())
					{
						osd.setMute(false);
						hardware.switch_mute();
					}
					else
					{
						osd.setMute(true);
						hardware.switch_mute();
					}
				}
				else if (key == RC1_GREEN)
				{
					int selectedeventid = osd.getSelectedSchedule();
					event tmp_event = eit.getEvent(selectedeventid);
					timer.addTimer(tmp_event.starttime, 2, tmp_event.event_name, tmp_event.duration = 0, channels.getCurrentChannelNumber());
					timer.saveTimer();
				}
				else if (key == RC1_DOWN)
				{
					osd.selectNextScheduleInformation();
				}
				else if (key == RC1_UP)
				{
					osd.selectPrevScheduleInformation();
				}
				else if (key == RC1_RIGHT)
				{
					osd.nextSchedulePage();
				}
				else if (key == RC1_LEFT)
				{
					osd.prevSchedulePage();
				}
				
				number = rc.get_number();
			} while(key != RC1_OK && key != RC1_HOME && key != RC1_HELP);
			osd.addCommand("HIDE schedule");;
			mode = switchmode;
			if (key == RC1_OK)
			{
				change = true;
				currentepg = 1;
				positionepg = 0;
				mode = 8;
				
				event tmp_event;
				int selectedeventid = osd.getSelectedSchedule();
				if (selectedeventid == 0)
				{
					mode = 2;
					continue;
				}
				tmp_event = eit.getEvent(selectedeventid);
				osd.createEPG();
				osd.setEPGEventName(tmp_event.event_name);
				osd.setEPGEventShortText(tmp_event.event_short_text);
				osd.setEPGEventExtendedText(tmp_event.event_extended_text);
				osd.setEPGProgramName(channels.getCurrentServiceName());
				osd.setEPGstarttime(tmp_event.starttime);
				osd.setEPGduration(tmp_event.duration);
				
			}
			break;
		case 13: // Box Setup
			osd.createMenu();
			osd.setMenuTitle("Box Setup");

			osd.addMenuEntry(1, "IP Setup");
			
			osd.addCommand("SHOW menu");

			osd.addCommand("COMMAND menu select next");
			
			leave = false;
			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DOWN)
					{
						osd.selectNextEntry();
					}
					if (key == RC1_UP)
					{
						osd.selectPrevEntry();
					}
					if (key == RC1_OK)
						number = osd.menuSelectedIndex();

					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				} while(key != RC1_OK && key != RC1_HOME && key != RC1_RIGHT && key != RC1_LEFT && key != RC1_BLUE && number == -1);
				
				if (key == RC1_OK)
				{
					number = osd.menuSelectedIndex();
				}
				
				leave = true;

				if (key == RC1_HOME || key == RC1_BLUE)
				{
					mode = switchmode;
				}
				else if (key == RC1_RIGHT || key == RC1_LEFT)
				{
					mode = 10;
				}
				else if (number == 1)
				{
					osd.addCommand("HIDE menu");
					mode = 14;

				}
				else
					leave = false;
			} while (!leave);
			osd.addCommand("HIDE menu");
			
			break;
		case 14: // IP Setup
			osd.createIP();
			osd.setIPDescription("Please enter IP-address!");
			osd.addCommand("SHOW ip");
			osd.addCommand("COMMAND ip position 0");
			
			do
			{
				key = rc.read_from_rc();
				number = rc.get_number();

				if (number != -1)
				{
					osd.setIP(number);
					osd.setIPNextPosition();
				}
				else if (key == RC1_RIGHT)
				{
					osd.setIPNextPosition();
				}
				else if (key == RC1_LEFT)
				{
					osd.setIPPrevPosition();
				}
			} while ( key != RC1_OK && key != RC1_HOME);
			mode = 13;
			if (key == RC1_OK)
			{
				settings.setIP(osd.getIPPart(0), osd.getIPPart(1), osd.getIPPart(2), osd.getIPPart(3));

				char command[100];
				sprintf(command, "ifconfig eth0 %d.%d.%d.%d", osd.getIPPart(0), osd.getIPPart(1), osd.getIPPart(2), osd.getIPPart(3));
				printf("%s\n", command);
				system(command);
				
				system(command);
			}

			osd.addCommand("HIDE ip");

			
			break;
		case 15: // PIDs
			osd.createMenu();
			osd.setMenuTitle("PIDs");
			
			sprintf(text, "VPID: %04x", channels.getCurrentVPID());
			osd.addMenuEntry(1, text);
			
			sprintf(text, "APID: %04x", channels.getCurrentAPID(0));
			osd.addMenuEntry(2, text);

			if (channels.getCurrentAPIDcount() > 1)
			{
				sprintf(text, "APID: %04x", channels.getCurrentAPID(1));
				osd.addMenuEntry(2, text);
			}

			sprintf(text, "SID: %04x", channels.getCurrentSID());
			osd.addMenuEntry(3, text);

			sprintf(text, "PMT: %04x", channels.getCurrentPMT());
			osd.addMenuEntry(4, text);

			sprintf(text, "TXT: %04x", channels.getCurrentTXT());
			osd.addMenuEntry(5, text);

			sprintf(text, "TS: %04x", channels.getCurrentTS());
			osd.addMenuEntry(6, text);

			sprintf(text, "ONID: %04x", channels.getCurrentONID());
			osd.addMenuEntry(7, text);

			sprintf(text, "PCR: %04x", channels.getCurrentPCR());
			osd.addMenuEntry(8, text);
			
			sprintf(text, "ECM: %04x", ECM);
			osd.addMenuEntry(9, text);

			sprintf(text, "Type: %04x", channels.getCurrentType());
			osd.addMenuEntry(10, text);
			
			sprintf(text, "CAID: %04x", settings.getCAID());
			osd.addMenuEntry(11, text);

			sprintf(text, "EMM: %04x", settings.getEMMpid(channels.getCurrentTS()));
			osd.addMenuEntry(12, text);

			osd.addCommand("SHOW menu");

			osd.addCommand("COMMAND menu select next");
			
			leave = false;
			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DOWN)
					{
						osd.selectNextEntry();
					}
					if (key == RC1_UP)
					{
						osd.selectPrevEntry();
					}
					if (key == RC1_OK)
						number = osd.menuSelectedIndex();
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				} while(key != RC1_OK && key != RC1_HOME && key != RC1_RIGHT && key != RC1_LEFT && key != RC1_BLUE && number == -1);
				
				if (key == RC1_OK)
				{
					number = osd.menuSelectedIndex();
				}
				
				leave = true;

				if (key == RC1_HOME || key == RC1_BLUE)
				{
					mode = switchmode;
				}
				else if (key == RC1_RIGHT || key == RC1_LEFT)
				{
					mode = 9;
				}
				else
					leave = false;
			} while (!leave);
			osd.addCommand("HIDE menu");
			
			break;
		case 16: // Plugins
			plugins.loadPlugins();
			osd.createMenu();
			osd.setMenuTitle("Plug-Ins");
			printf ("NOP: %d\n", plugins.getNumberOfPlugins());
			for (int i = 0; i < plugins.getNumberOfPlugins(); i++)
			{
				
				osd.addMenuEntry(i + 1, plugins.getName(i));
			}
			osd.addCommand("SHOW menu");
			osd.addCommand("COMMAND menu select next");

			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DOWN)
					{
						osd.selectNextEntry();
					}
					if (key == RC1_UP)
					{
						osd.selectPrevEntry();
					}
					if (key == RC1_OK)
						number = osd.menuSelectedIndex();
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				} while (key != RC1_OK && key != RC1_HOME && key != RC1_RIGHT && key != RC1_LEFT && key != RC1_BLUE && number == -1);
				if (key == RC1_OK)
				{
					number = osd.menuSelectedIndex();
				}
				
				leave = true;
				if (key == RC1_HOME || key == RC1_BLUE)
				{
					mode = switchmode;
				}
				else if (key == RC1_RIGHT || key == RC1_LEFT)
				{
					mode = 9;
				}
				else if (number <= plugins.getNumberOfPlugins() && number > 0)
				{
					if (plugins.getShowPig(number - 1))
					{
						pig.hide();	
						pig.setSize(plugins.getSizeX(number - 1), plugins.getSizeY(number - 1));
						pig.setStack(1);
						pig.show();
						pig.setPosition(plugins.getPosX(number - 1), plugins.getPosY(number - 1));
					}

					plugins.setfb(fb.getHandle());
					plugins.setrc(rc.getHandle());
					printf("Handle: %d\n", rc.getHandle());
					plugins.setlcd(-1);
					plugins.startPlugin(number - 1);
					if (plugins.getShowPig(number - 1))
					{
						pig.hide();
					}
					rc.restart();
					usleep(400000);
					fb.clearScreen();
				
				}
				else
					leave = false;
			} while (!leave);
			osd.addCommand("HIDE menu");
	
			

			break;
		case 17: // Timer
			osd.createMenu();
			osd.setMenuTitle("Timer");
			osd.addCommand("COMMAND menu set_size 200");

			timer.dumpTimer();

			osd.addCommand("SHOW menu");

			osd.addCommand("COMMAND menu select next");
			
			leave = false;
			do
			{
				do
				{
					key = rc.read_from_rc();
					number = rc.get_number();
					if (key == RC1_DOWN)
					{
						osd.selectNextEntry();
					}
					if (key == RC1_UP)
					{
						osd.selectPrevEntry();
					}
					if (key == RC1_OK)
						number = osd.menuSelectedIndex();
					else if (key == RC1_MUTE)
					{
						if (hardware.isMuted())
						{
							osd.setMute(false);
							hardware.switch_mute();
						}
						else
						{
							osd.setMute(true);
							hardware.switch_mute();
						}
					}
				} while(key != RC1_OK && key != RC1_HOME && key != RC1_RIGHT && key != RC1_LEFT && key != RC1_BLUE && number == -1 && key != RC1_RED);
				
				if (key == RC1_OK)
				{
					number = osd.menuSelectedIndex();
				}
				else if (key == RC1_RED)
				{
					number = osd.menuSelectedIndex();
					timer.rmTimer(timer.getDumpedChannel(number), timer.getDumpedStarttime(number));
					timer.saveTimer();
					osd.addCommand("HIDE menu");
					osd.createMenu();
					osd.setMenuTitle("Timer");
					osd.addCommand("COMMAND menu set_size 200");

					timer.dumpTimer();
					
					osd.addCommand("SHOW menu");
					osd.addCommand("COMMAND menu select next");
				}

				leave = true;

				if (key == RC1_HOME || key == RC1_BLUE)
				{
					mode = switchmode;
				}
				else if (key == RC1_RIGHT || key == RC1_LEFT)
				{
					mode = 9;
				}
				else
					leave = false;
			} while (!leave);
			osd.addCommand("HIDE menu");
			
			break;

		}

	} while (key != RC1_STANDBY);
	hardware.shutdown();
}
