/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
                      2003 thegoodguy
	Copyright (C) 2008 Novell, Inc. Author: Stefan Seyfried

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/rcinput.h>
#include <driver/stream2file.h>

#ifdef HAVE_TRIPLEDRAGON
#include <tdpanel/ir_ruwido.h>
#include <hardware/avs/avs_inf.h>
#include <hardware/avs/bios_system_config.h>
#endif
#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
#include <termio.h>
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
#include <unistd.h>
#include <fcntl.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <errno.h>

#include <eventserver.h>

#include <global.h>
#include <neutrino.h>
#include <fstream>		// used for reading conf file

#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {"/dev/rawir2"};
#define RC_standby_release (KEY_MAX + 1)
typedef struct { __u16 code; } t_input_event;
#elif defined(HAVE_TRIPLEDRAGON)
const char *const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {"/dev/stb/tdremote"};
typedef struct { __u16 code; } t_input_event;
#else /* DBOX */
#ifdef OLD_RC_API
const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {"/dev/dbox/rc0"};
#define RC_standby_release (KEY_MAX + 1)
typedef struct { __u16 code; } t_input_event;
#else /* OLD_RC_API */
const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {"/dev/input/event0", "/dev/input/event1"};
typedef struct input_event t_input_event;
#endif /* OLD_RC_API */
#endif /* HAVE_DREAMBOX_HARDWARE */


#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
static struct termio orig_termio;
static bool          saved_orig_termio = false;
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */


#define SYSTEM		0xFFF00000
#define RELOAD_CONF	SYSTEM+1
#define MODE_TV		SYSTEM+2
#define MODE_RADIO	SYSTEM+3

const char CRCInput::key_modifiers[] = " ru";

const CRCInput::key CRCInput::keyname[] = {
	{"rc_0",	 	RC_0},
	{"rc_1",	 	RC_1},
	{"rc_2",	 	RC_2},
	{"rc_3",	 	RC_3},
	{"rc_4",	 	RC_4},
	{"rc_5",	 	RC_5},
	{"rc_6",	 	RC_6},
	{"rc_7",	 	RC_7},
	{"rc_8",	 	RC_8},
	{"rc_9",	 	RC_9},
	{"rc_right",		RC_right},
	{"rc_left",		RC_left},
	{"rc_up",		RC_up},
	{"rc_down",		RC_down},
	{"rc_ok",		RC_ok},
	{"rc_spkr",		RC_spkr},    // !!
	{"rc_standby",		RC_standby}, // !!
	{"rc_green",		RC_green},
	{"rc_yellow",		RC_yellow},
	{"rc_red",		RC_red},
	{"rc_blue",		RC_blue},
	{"rc_minus",		RC_minus}, // !!
	{"rc_plus",		RC_plus},  // !!
	{"rc_help",		RC_help},
	{"rc_setup",		RC_setup},
	{"rc_top_left",		RC_top_left},
	{"rc_top_right",	RC_top_right},
	{"rc_bottom_left",	RC_bottom_left},
	{"rc_bottom_right",	RC_bottom_right},
	{"rc_tv",		RC_tv},
	{"rc_radio",		RC_radio},
	{"rc_text",		RC_text},
	{"rc_audio",		RC_audio},
	{"rc_video",		RC_video},
	{"rc_next",		RC_next},
	{"rc_prev",		RC_prev},
	{"rc_aux",		RC_aux},
	{"rc_timer",		RC_timer},
	{"rc_epg",		RC_epg},
	{"rc_fav",		RC_fav},
	{"rc_tttv",		RC_tttv},
	{"rc_ttzoom",		RC_ttzoom},
	{"rc_ttreveal",		RC_ttreveal},
	{"rc_playpause",	RC_playpause},
	{"rc_stop",		RC_stop},
	{"rc_eject",		RC_eject},
	{"rc_forward",		RC_forward},
	{"rc_back",		RC_back},
	{"rc_record",		RC_record},
	{"rc_zoomin",		RC_zoomin},
	{"rc_zoomout",		RC_zoomout},
	{"rc_zoomoff",		RC_zoomoff},
	{"rc_home",		RC_home},
	{"rc_page_down",	RC_page_down},
	{"rc_page_up",		RC_page_up},
	{"rc_esc",		RC_esc},
	{"rc_hyphen",		RC_hyphen}, // !!
	{"rc_equal",		RC_equal},
	{"rc_backspace",	RC_backspace},
	{"rc_tab",		RC_tab},
	{"rc_q",		RC_q},
	{"rc_w",		RC_w},
	{"rc_e",		RC_e},
	{"rc_r",		RC_r},
	{"rc_t",		RC_t},
	{"rc_y",		RC_y},
	{"rc_u",		RC_u},
	{"rc_i",		RC_i},
	{"rc_o",		RC_o},
	{"rc_p",		RC_p},
	{"rc_leftbrace",	RC_leftbrace},
	{"rc_rightbrace",	RC_rightbrace},
	{"rc_enter",		RC_enter},
	{"rc_leftctrl",		RC_leftctrl},
	{"rc_a",		RC_a},
	{"rc_s",		RC_s},
	{"rc_d",		RC_d},
	{"rc_f",		RC_f},
	{"rc_g",		RC_g},
	{"rc_h",		RC_h},
	{"rc_j",		RC_j},
	{"rc_k",		RC_k},
	{"rc_l",		RC_l},
	{"rc_semicolon",	RC_semicolon},
	{"rc_apostrophe",	RC_apostrophe},
	{"rc_grave",		RC_grave},
	{"rc_leftshift",	RC_leftshift},
	{"rc_backslash",	RC_backslash},
	{"rc_z",		RC_z},
	{"rc_x",		RC_x},
	{"rc_c",		RC_c},
	{"rc_v",		RC_v},
	{"rc_b",		RC_b},
	{"rc_n",		RC_n},
	{"rc_m",		RC_m},
	{"rc_comma",		RC_comma}, 	
	{"rc_dot",		RC_dot}, 		
	{"rc_slash",		RC_slash}, 	
	{"rc_rightshift",	RC_rightshift}, 	
	{"rc_kpasterisk",	RC_kpasterisk}, 	
	{"rc_leftalt",		RC_leftalt}, 	
	{"rc_space",		RC_space}, 	
	{"rc_capslock",		RC_capslock}, 
	{"rc_f1",		RC_f1},
	{"rc_f2",		RC_f2}, 	
	{"rc_f3",		RC_f3}, 	
	{"rc_f4",		RC_f4}, 	
	{"rc_f5",		RC_f5}, 	
	{"rc_f6",		RC_f6}, 	
	{"rc_f7",		RC_f7}, 	
	{"rc_f8",		RC_f8}, 	
	{"rc_f9",		RC_f9}, 	
	{"rc_f10",		RC_f10}, 	 
	{"rc_numlock",		RC_numlock}, 	 
	{"rc_scrolllock",	RC_scrolllock}, 
	{"rc_kp7",		RC_kp7},
	{"rc_kp8",		RC_kp8},	
	{"rc_kp9",		RC_kp9},		
	{"rc_kpminus",		RC_kpminus},		
	{"rc_kp4",		RC_kp4},
	{"rc_kp5",		RC_kp5},	
	{"rc_kp6",		RC_kp6},	
	{"rc_kpplus",		RC_kpplus},	
	{"rc_kp1",		RC_kp1},		
	{"rc_kp2",		RC_kp2},		
	{"rc_kp3",		RC_kp3},		
	{"rc_kp0",		RC_kp0},		
	{"rc_kpdot",		RC_kpdot},		
	{"rc_102nd",		RC_102nd}, 	 	
	{"rc_kpenter",		RC_kpenter},	
	{"rc_kpslash",		RC_kpslash},	
	{"rc_sysrq",		RC_sysrq}, 
	{"rc_rightalt",		RC_rightalt}, 
	{"rc_end",		RC_end}, 	 
	{"rc_insert",		RC_insert}, 
	{"rc_delete",		RC_delete}, 
	{"rc_pause",		RC_pause}, 	
	{"rc_leftmeta",		RC_leftmeta},
	{"rc_rightmeta",	RC_rightmeta},
	{"rc_btn_left",		RC_btn_left}, 
	{"rc_btn_right",	RC_btn_right},

	{"system",		SYSTEM},
	{"reload_conf",		RELOAD_CONF},
	{"mode_radio",		MODE_RADIO},
	{"mode_tv",		MODE_TV},


	{"show_epg",		NeutrinoMessages::SHOW_EPG},
	{"show_infobar",	NeutrinoMessages::SHOW_INFOBAR},
	{"vcr_on",		NeutrinoMessages::VCR_ON},
	{"vcr_off",		NeutrinoMessages::VCR_OFF},
	{"standby_on",		NeutrinoMessages::STANDBY_ON},
	{"standby_off",		NeutrinoMessages::STANDBY_OFF},
	//{"standby_toggle",	NeutrinoMessages::STANDBY_TOGGLE},
	{"shutdown",		NeutrinoMessages::SHUTDOWN},
	//{"announce_shutdown", NeutrinoMessages::ANNOUNCE_SHUTDOWN},
	//{"announce_zapto",	NeutrinoMessages::ANNOUNCE_ZAPTO},
	//{"zapto",		NeutrinoMessages::ZAPTO},
	//{"announce_record",	NeutrinoMessages::ANNOUNCE_RECORD},
	//{"record_start",	NeutrinoMessages::RECORD_START},
	//{"record_stop",	NeutrinoMessages::RECORD_STOP},
	//{"announce_sleeptimer", NeutrinoMessages::ANNOUNCE_SLEEPTIMER},
	//{"sleeptimer",	NeutrinoMessages::SLEEPTIMER},
	//{"changemode",	NeutrinoMessages::CHANGEMODE},
	//{"remind",		NeutrinoMessages::REMIND},
	{"lock_rc",		NeutrinoMessages::LOCK_RC},
	{"unlock_rc",		NeutrinoMessages::UNLOCK_RC},
	{"esound_on",		NeutrinoMessages::ESOUND_ON},
	{"esound_off",		NeutrinoMessages::ESOUND_OFF},

	{"show_volume",		NeutrinoMessages::EVT_VOLCHANGED},
	//{"evt_mutechanged",	NeutrinoMessages::EVT_MUTECHANGED},
	//{"evt_vcrchanged",	NeutrinoMessages::EVT_VCRCHANGED},
	//{"evt_modechanged",	NeutrinoMessages::EVT_MODECHANGED},
	//{"evt_bouquetschanged", NeutrinoMessages::EVT_BOUQUETSCHANGED},
        //{"evt_serviceschanged", EVT_SERVICESCHANGED},

	//{"evt_scan_complete",	 NeutrinoMessages::EVT_SCAN_COMPLETE},
	//{"evt_scan_num_transponders",	 NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS},
	//{"evt_scan_num_channels",	 NeutrinoMessages::EVT_SCAN_NUM_CHANNELS},
	//{"evt_shutdown",	NeutrinoMessages::EVT_SHUTDOWN},
	//{"evt_timer",		NeutrinoMessages::EVT_TIMER},
	//{"evt_programlockstatus",	 NeutrinoMessages::EVT_PROGRAMLOCKSTATUS},
	//{"evt_recordmode",	NeutrinoMessages::EVT_RECORDMODE},
	//{"evt_zap_ca_clear",	NeutrinoMessages::EVT_ZAP_CA_CLEAR},
	//{"evt_zap_ca_lock",	NeutrinoMessages::EVT_ZAP_CA_LOCK},
	//{"evt_zap_ca_fta",	NeutrinoMessages::EVT_ZAP_CA_FTA},
	//{"evt_scan_failed",	NeutrinoMessages::EVT_SCAN_FAILED},
	//{"evt_scan_report_num_scanned_transponders",	 NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS},
	//{"evt_scan_report_frequency",	 NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY},
	//{"evt_scan_found_radio_chan",	 NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN},
	//{"evt_scan_found_data_chan",		NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN},
	//{"evt_scan_found_tv_chan",	 NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN},
	//{"evt_scan_report_frequencyp",	 NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP},
	//{"evt_zap_motor",	NeutrinoMessages::EVT_ZAP_MOTOR},
				/* sectionsd */
	//{"evt_services_upd",	NeutrinoMessages::EVT_SERVICES_UPD},
	//{"evt_si_finished",	NeutrinoMessages::EVT_SI_FINISHED},


	//{"evt_currentepg",	NeutrinoMessages::EVT_CURRENTEPG},
	//{"evt_nextepg",	NeutrinoMessages::EVT_NEXTEPG},

	{"evt_popup",		NeutrinoMessages::EVT_POPUP},
	{"evt_extmsg",		NeutrinoMessages::EVT_EXTMSG},
	{"evt_start_plugin",	NeutrinoMessages::EVT_START_PLUGIN},

				/* sectionsd */
	//{"evt_currentnext_epg",	 NeutrinoMessages::EVT_CURRENTNEXT_EPG},
	//{"evt_timeset",		NeutrinoMessages::EVT_TIMESET},

				/* "sectionsd" events triggered by neutrino */
	//{"evt_noepg_yet",	NeutrinoMessages::EVT_NOEPG_YET},

				/* "timerd" events triggered by neutrino */
	//{"evt_nextprogram",	NeutrinoMessages::EVT_NEXTPROGRAM},

				/* zapit */
	//{"evt_scan_found_a_chan",	 NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN},
	//{"evt_scan_provider",	 NeutrinoMessages::EVT_SCAN_PROVIDER},
	//{"evt_scan_satellite", NeutrinoMessages::EVT_SCAN_SATELLITE},
	//{"evt_scan_servicename",	 NeutrinoMessages::EVT_SCAN_SERVICENAME},
	//{"evt_zap_complete",	NeutrinoMessages::EVT_ZAP_COMPLETE},
	//{"evt_zap_failed",	NeutrinoMessages::EVT_ZAP_FAILED},
	//{"evt_zap_isnvod",	NeutrinoMessages::EVT_ZAP_ISNVOD},
	//{"evt_zap_sub_complete",	 NeutrinoMessages::EVT_ZAP_SUB_COMPLETE},
	//{"evt_zap_sub_failed", NeutrinoMessages::EVT_ZAP_SUB_FAILED},

				/* "zapit" events triggered by neutrino */
	//{"evt_zap_got_subservices",		NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES},
	//{"evt_zap_gotapids",	NeutrinoMessages::EVT_ZAP_GOTAPIDS},
	//{"evt_zap_gotpids",	NeutrinoMessages::EVT_ZAP_GOTPIDS},

				/* neutrino */
	//{"evt_recording_ended",	 NeutrinoMessages::EVT_RECORDING_ENDED},

	{"rc_null",		CRCInput::RC_ignore /* was: 0xFFFFFFFD*/},
	{"rc_none",		CRCInput::RC_nokey /* was: 0xFFFFFFFF*/}
};

/**************************************************************************
*	Constructor - opens rc-input device and starts threads
*
**************************************************************************/
CRCInput::CRCInput()
{
	timerid= 1;

	// pipe for internal event-queue
	// -----------------------------
	if (pipe(fd_pipe_high_priority) < 0)
	{
		perror("fd_pipe_high_priority");
		exit(-1);
	}

	fcntl(fd_pipe_high_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_high_priority[1], F_SETFL, O_NONBLOCK );

	if (pipe(fd_pipe_low_priority) < 0)
	{
		perror("fd_pipe_low_priority");
		exit(-1);
	}

	fcntl(fd_pipe_low_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_low_priority[1], F_SETFL, O_NONBLOCK );


	// open event-library
	// -----------------------------
	fd_event = 0;

	//network-setup
	struct sockaddr_un servaddr;
	int    clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, NEUTRINO_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(NEUTRINO_UDS_NAME);

	//network-setup
	if ((fd_event = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
	perror("[neutrino] socket\n");
	}

	if ( bind(fd_event, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		perror("[neutrino] bind failed...\n");
		exit(-1);
	}

#define N_connection_requests_queued 10

	if (listen(fd_event, N_connection_requests_queued) !=0)
	{
		perror("[neutrino] listen failed...\n");
		exit( -1 );
	}

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		fd_rc[i] = -1;
	}

	load_conf(true);

	open();
}

uint CRCInput::modch2int(char c)
{
	for (uint i = 1; i < no_modifiers; i++)
		if (c == key_modifiers[i])
			return i;

	return 0;
}

char CRCInput::modint2ch(uint i)
{
	if (i >= no_modifiers) {
		printf("[rcinput] modint2ch called with invalid argument = %d\n", i);
		return '?';
	}
	return key_modifiers[i];
}

void CRCInput::load_conf(bool initialize) {
	debug_user_translate = false;
	no_neutrinoevents_when_vc = false;

	if (!initialize)
	{
		for (uint i = 0; i < no_modifiers; i++)
		{
			for (std::map<uint, neutrino_msg_data_t>::iterator it = user_translate_data[i].begin();
			     it != user_translate_data[i].end();
			     it++)
				free((void*)it->second);

			user_translate_data[i].clear();
			user_translate_table[i].clear();
		}
	}

	std::ifstream rc_conf(RC_CONF_PATH);
	if (rc_conf.good())
	{
		printf("[rcinput] found "  RC_CONF_PATH ", parsing...\n");
		std::string line, keyword, argument, action, data;
		while (getline(rc_conf, line))
		{
			if (line[0] != '#') {	// Skip comments
				int s = line.find("=");
				int end_keyword = s;
				if (s != -1)
				{
					uint modifier = 0;
					int left_bracket = line.substr(0,s).find("[");
					if (left_bracket != -1)
					{
						end_keyword = left_bracket;
						char modifier_ch = line[left_bracket+1];
						modifier = modch2int(modifier_ch);
					}
					keyword = trim(line.substr(0, end_keyword));
					argument = line.substr(s+1);
					if (argument.substr(0,7) == "system(")
						s = argument.find_first_of("#");
					else
						s = argument.find_first_of(";#");
					argument = trim(argument.substr(0,s));
					s = argument.find_first_of("(");
					if (s != -1)
					{
						action = argument.substr(0, s);
						data = argument.substr(s+1);
						s = data.find_last_of(")");
						data = trim(data.substr(0,s));
					}
					else
					{
						action = argument;
						data = "";
					}
					uint keycode = keyname2keycode(keyword);

					if (keycode != RC_nokey && user_translate_table[modifier].find(keycode) != user_translate_table[modifier].end())
					  printf("[rcinput] Warning: overriding previous binding of %s[%c]\n", keyword.c_str(), modint2ch(modifier));
	  
					if (keyword == "debug")
					{
						if (argument == "dump")
							debug_dump();
						else
						{
							debug_user_translate = (argument == "on" || argument == "yes");
							printf("[rcinput] debug_user_translate is now %s\n", argument.c_str());
						}
					}
					else if (keyword == "no_neutrinoevents_when_vc")
					{
						no_neutrinoevents_when_vc = (argument == "on" || argument == "yes");
						printf("[rcinput] no_neutrinoevents_when_vc is now %s\n", argument.c_str());
					}
					else if (keycode == SYSTEM)
					{
						char *buf = (char *) malloc((data.length()+1)*sizeof(char));
						strcpy(buf, data.c_str());
						user_translate_data[modifier][keycode] = (neutrino_msg_data_t) buf;
					}
					else if (keycode != CRCInput::RC_nokey)
					{
					

					  uint keyaction = keyname2keycode(action);
						if (keyaction != CRCInput::RC_nokey)
						{
							if (debug_user_translate) {
								if (data == "")
								  printf("[rcinput] Binding %s[%c] -> %s\n", keyword.c_str(), modint2ch(modifier), argument.c_str());
								else
									printf("[rcinput] Binding %s[%c] -> %s(%s)\n", keyword.c_str(), modint2ch(modifier), action.c_str(), data.c_str());
							}
							user_translate_table[modifier][keycode] = (neutrino_msg_t) keyaction;
							if (data != "")
							{
								char *data_str = (char *) malloc((data.length()+1)*sizeof(char));
								strcpy(data_str, data.c_str());
								user_translate_data[modifier][keycode] = (neutrino_msg_data_t) data_str;
							}
							else
								user_translate_data[modifier][keycode] = (neutrino_msg_data_t) NULL;
						}
						else
							//if (debug_user_translate)
						        printf("[rcinput] Key Action `%s' not recognized\n", argument.c_str());
					}
					else
						//if (debug_user_translate)
						printf("[rcinput] Key `%s' not recognized\n", keyword.c_str());
				}
			}
		}
	}
	else
		printf("[rcinput] no file "  RC_CONF_PATH " was found\n");
}

void CRCInput::debug_dump()
{
	for (uint i = 0; i < no_modifiers; i++)
	{
		printf("[rcinput/debug_dump] Content of user_translate_table/user_translate_data[%c]:\n", modint2ch(i));
		for (std::map<uint, neutrino_msg_t>::iterator it = user_translate_table[i].begin();
		     it != user_translate_table[i].end(); it ++)
		{
			uint keycode = it->first;
			std::map<uint, neutrino_msg_data_t>::iterator dat = user_translate_data[i].find(keycode); 
			printf("[rcinput/debug_dump] %s\t%s\t%s\n",
			       keycode2keyname(keycode),
			       keycode2keyname((uint)it->second),
			       dat == user_translate_data[i].end() ? ""
			       //: dat->second == 1 ? "mode_TV"
			       //: dat->second == 2 ? "mode_Radio"
			       : (char *) dat->second);
		}
	}
}

unsigned CRCInput::keyname2keycode(std::string the_keyname)
{
	unsigned int i = 0;
	while (the_keyname != keyname[i].name && keyname[i].code != RC_nokey)
		i++;
	return keyname[i].code;
}

const char *CRCInput::keycode2keyname(unsigned the_keycode)
{
	unsigned int i = 0;
	while (the_keycode != keyname[i].code && keyname[i].code != RC_nokey)
		i++;
	return keyname[i].name;
}

std::string CRCInput::trim(std::string s)
{
	const char *whitespace = " \t\r\n";
	int beg = s.find_first_not_of(whitespace);
	if (beg == -1)
		return "";
	int end = s.find_last_not_of(whitespace);
	return s.substr(beg, end+1);
}

void CRCInput::open()
{
	close();

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if ((fd_rc[i] = ::open(RC_EVENT_DEVICE[i], O_RDONLY)) == -1)
			perror(RC_EVENT_DEVICE[i]);
		else
		{
#ifdef OLD_RC_API
			ioctl(fd_rc[i], RC_IOCTL_BCODES, 1);
#endif /* OLD_RC_API */
			fcntl(fd_rc[i], F_SETFL, O_NONBLOCK);
		}
	}

	//+++++++++++++++++++++++++++++++++++++++
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	fd_keyb = STDIN_FILENO;
#else
	fd_keyb = 0;
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
	/*
	::open("/dev/dbox/rc0", O_RDONLY);
	if (fd_keyb<0)
	{
		perror("/dev/stdin");
		exit(-1);
	}
	*/
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	::fcntl(fd_keyb, F_SETFL, O_NONBLOCK);

	struct termio new_termio;

	::ioctl(STDIN_FILENO, TCGETA, &orig_termio);

	saved_orig_termio      = true;

	new_termio             = orig_termio;
	new_termio.c_lflag    &= ~ICANON;
	//	new_termio.c_lflag    &= ~(ICANON|ECHO);
	new_termio.c_cc[VMIN ] = 1;
	new_termio.c_cc[VTIME] = 0;

	::ioctl(STDIN_FILENO, TCSETA, &new_termio);

#else
	//fcntl(fd_keyb, F_SETFL, O_NONBLOCK );

	//+++++++++++++++++++++++++++++++++++++++
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

	calculateMaxFd();
}

void CRCInput::close()
{
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			::close(fd_rc[i]);
			fd_rc[i] = -1;
		}
	}
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	if (saved_orig_termio)
	{
		::ioctl(STDIN_FILENO, TCSETA, &orig_termio);
		printf("Original terminal settings restored.\n");
	}
#else
/*
	if(fd_keyb)
	{
		::close(fd_keyb);
	}
*/
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

	calculateMaxFd();
}

void CRCInput::calculateMaxFd()
{
	fd_max = fd_event;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		if (fd_rc[i] > fd_max)
			fd_max = fd_rc[i];
	
	if(fd_pipe_high_priority[0] > fd_max)
		fd_max = fd_pipe_high_priority[0];
	if(fd_pipe_low_priority[0] > fd_max)
		fd_max = fd_pipe_low_priority[0];
}


/**************************************************************************
*	Destructor - close the input-device
*
**************************************************************************/
CRCInput::~CRCInput()
{
	close();

	if(fd_pipe_high_priority[0])
		::close(fd_pipe_high_priority[0]);
	if(fd_pipe_high_priority[1])
		::close(fd_pipe_high_priority[1]);

	if(fd_pipe_low_priority[0])
		::close(fd_pipe_low_priority[0]);
	if(fd_pipe_low_priority[1])
		::close(fd_pipe_low_priority[1]);

	if(fd_event)
		::close(fd_event);
}

/**************************************************************************
*	stopInput - stop reading rcin for plugins
*
**************************************************************************/
void CRCInput::stopInput()
{
	close();
}


/**************************************************************************
*	restartInput - restart reading rcin after calling plugins
*
**************************************************************************/
void CRCInput::restartInput()
{
	open();
}

int CRCInput::messageLoop( bool anyKeyCancels, int timeout )
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	bool doLoop = true;

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_MENU];

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	while (doLoop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

	if ( ( msg == CRCInput::RC_timeout ) ||
		( msg == CRCInput::RC_home ) ||
		( msg == CRCInput::RC_ok ) )
			doLoop = false;
		else
		{
			int mr = CNeutrinoApp::getInstance()->handleMsg( msg, data );

			if ( mr & messages_return::cancel_all )
			{
				res = menu_return::RETURN_EXIT_ALL;
				doLoop = false;
			}
			else if ( mr & messages_return::unhandled )
			{
				if ((msg <= CRCInput::RC_MaxRC) &&
				    (data == 0))                     /* <- button pressed */
				{
					if ( anyKeyCancels )
						doLoop = false;
					else
						timeoutEnd = CRCInput::calcTimeoutEnd( timeout );
				}
			}
		}


	}
	return res;
}


int CRCInput::addTimer(unsigned long long Interval, bool oneshot, bool correct_time )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	timer _newtimer;
	if (!oneshot)
		_newtimer.interval = Interval;
	else
		_newtimer.interval = 0;

	_newtimer.id = timerid++;
	if ( correct_time )
		_newtimer.times_out = timeNow+ Interval;
	else
		_newtimer.times_out = Interval;

	_newtimer.correct_time = correct_time;

	//printf("adding timer %d (0x%llx, 0x%llx)\n", _newtimer.id, _newtimer.times_out, Interval);

	std::vector<timer>::iterator e;
	for ( e= timers.begin(); e!= timers.end(); ++e )
		if ( e->times_out> _newtimer.times_out )
			break;

	timers.insert(e, _newtimer);
	return _newtimer.id;
}

int CRCInput::addTimer(struct timeval Timeout)
{
	unsigned long long timesout = (unsigned long long) Timeout.tv_usec + (unsigned long long)((unsigned long long) Timeout.tv_sec * (unsigned long long) 1000000);
	return addTimer( timesout, true, false );
}

int CRCInput::addTimer(const time_t *Timeout)
{
	return addTimer( (unsigned long long)*Timeout* (unsigned long long) 1000000, true, false );
}

void CRCInput::killTimer(uint id)
{
	//printf("killing timer %d\n", id);
	std::vector<timer>::iterator e;
	for ( e= timers.begin(); e!= timers.end(); ++e )
		if ( e->id == id )
		{
			timers.erase(e);
			break;
		}
}

int CRCInput::checkTimers()
{
	struct timeval tv;
	int _id = 0;

	gettimeofday( &tv, NULL );
	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	std::vector<timer>::iterator e;
	for ( e= timers.begin(); e!= timers.end(); ++e )
		if ( e->times_out< timeNow+ 2000 )
		{
//			printf("timeout timer %d %llx %llx\n",e->id,e->times_out,timeNow );
			_id = e->id;
			if ( e->interval != 0 )
			{
				timer _newtimer;
				_newtimer.id = e->id;
				_newtimer.interval = e->interval;
				_newtimer.correct_time = e->correct_time;
				if ( _newtimer.correct_time )
					_newtimer.times_out = timeNow + e->interval;
				else
					_newtimer.times_out = e->times_out + e->interval;

				timers.erase(e);
				for ( e= timers.begin(); e!= timers.end(); ++e )
					if ( e->times_out> _newtimer.times_out )
						break;

				timers.insert(e, _newtimer);
			}
			else
				timers.erase(e);

			break;
        }
//        else
//    		printf("skipped timer %d %llx %llx\n",e->id,e->times_out, timeNow );
	return _id;
}



long long CRCInput::calcTimeoutEnd(const int timeout_in_seconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return timeout_in_seconds > 0 ? (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec + (unsigned long long)timeout_in_seconds) * (unsigned long long) 1000000 : (unsigned long long) -1;
}

long long CRCInput::calcTimeoutEnd_MS(const int timeout_in_milliseconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	return ( timeNow + timeout_in_milliseconds * 1000 );
}


void CRCInput::getMsgAbsoluteTimeout(neutrino_msg_t *msg, neutrino_msg_data_t *data, unsigned long long *TimeoutEnd)
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	unsigned long long diff;

	if ( *TimeoutEnd < timeNow+ 100 )
		diff = 100;  // Minimum Differenz...
	else
		diff = ( *TimeoutEnd - timeNow );

	getMsg_us(msg, data, diff);

	if ( *msg == NeutrinoMessages::EVT_TIMESET )
	{
		// recalculate timeout....
		//unsigned long long ta= *TimeoutEnd;
		*TimeoutEnd= *TimeoutEnd + *(long long*) *data;

		//printf("[getMsgAbsoluteTimeout]: EVT_TIMESET - recalculate timeout\n%llx/%llx - %llx/%llx\n", timeNow, *(long long*) *data, *TimeoutEnd, ta );
	}
}

void CRCInput::getMsg(neutrino_msg_t *msg, neutrino_msg_data_t *data, int Timeout)
{
	getMsg_us(msg, data, Timeout * 100 * 1000ULL);
}

void CRCInput::getMsg_ms(neutrino_msg_t *msg, neutrino_msg_data_t *data, int Timeout)
{
	getMsg_us(msg, data, Timeout * 1000ULL);
}

void CRCInput::getMsg_us(neutrino_msg_t *msg, neutrino_msg_data_t *data, unsigned long long Timeout)
{
	static unsigned long long last_keypress = 0ULL;
	unsigned long long getKeyBegin;

#if defined(OLD_RC_API) || defined(HAVE_TRIPLEDRAGON)
	static int rc_last_key = 0;
#endif
	static bool repeating = false;

	struct timeval tv, tvselect;
	unsigned long long InitialTimeout = Timeout;
	long long targetTimeout;

	int timer_id;
	fd_set rfds;
	t_input_event ev;

	//set 0
	*data = 0;

	// wiederholung reinmachen - dass wirklich die ganze zeit bis timeout gewartet wird!
	gettimeofday( &tv, NULL );
	getKeyBegin = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	while(1)
	{
		timer_id = 0;
		if ( timers.size()> 0 )
		{
			gettimeofday( &tv, NULL );
			unsigned long long t_n= (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
			if ( timers[0].times_out< t_n )
			{
				timer_id = checkTimers();
			*msg = NeutrinoMessages::EVT_TIMER;
				*data = timer_id;
				return;
			}
			else
			{
				targetTimeout = timers[0].times_out - t_n;
				if ( (unsigned long long) targetTimeout> Timeout)
					targetTimeout= Timeout;
				else
					timer_id = timers[0].id;
			}
		}
		else
			targetTimeout= Timeout;

		tvselect.tv_sec = targetTimeout/1000000;
		tvselect.tv_usec = targetTimeout%1000000;
		//printf("InitialTimeout= %lld:%lld\n", Timeout/1000000,Timeout%1000000);
	        //printf("targetTimeout= %d:%d\n", tvselect.tv_sec,tvselect.tv_usec);

		FD_ZERO(&rfds);
		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if (fd_rc[i] != -1)
				FD_SET(fd_rc[i], &rfds);
		}
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
		if (true)
#else
		if (fd_keyb> 0)
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
			FD_SET(fd_keyb, &rfds);

		FD_SET(fd_event, &rfds);
		FD_SET(fd_pipe_high_priority[0], &rfds);
		FD_SET(fd_pipe_low_priority[0], &rfds);

		int status =  select(fd_max+1, &rfds, NULL, NULL, &tvselect);

		if ( status == -1 )
		{
			perror("[neutrino - getMsg_us]: select returned ");
			// in case of an error return timeout...?!
			*msg = RC_timeout;
			*data = 0;
			return;
		}
		else if ( status == 0 ) // Timeout!
		{
			if ( timer_id != 0 )
			{
			    timer_id = checkTimers();
				if ( timer_id != 0 )
				{
        			*msg = NeutrinoMessages::EVT_TIMER;
					*data = timer_id;
					return;
				}
				else
					continue;
			}
			else
			{
				*msg = RC_timeout;
				*data = 0;
				return;
			}
		}

		if(FD_ISSET(fd_pipe_high_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_high_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			// printf("got event from high-pri pipe %x %x\n", *msg, *data );

			return;
		}


#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
		if (FD_ISSET(fd_keyb, &rfds))
		{
			int trkey;
			char key = 0;
			read(fd_keyb, &key, sizeof(key));

			switch(key)
			{
			case 27: // <- Esc
				trkey = KEY_HOME;
				break;
			case 10: // <- Return
			case 'o':
				trkey = KEY_OK;
				break;
			case 'p':
				trkey = KEY_POWER;
				break;
			case 's':
				trkey = KEY_SETUP;
				break;
			case 'h':
				trkey = KEY_HELP;
				break;
			case 'i':
				trkey = KEY_UP;
				break;
			case 'm':
				trkey = KEY_DOWN;
				break;
			case 'j':
				trkey = KEY_LEFT;
				break;
			case 'k':
				trkey = KEY_RIGHT;
				break;
			case 'r':
				trkey = KEY_RED;
				break;
			case 'g':
				trkey = KEY_GREEN;
				break;
			case 'y':
				trkey = KEY_YELLOW;
				break;
			case 'b':
				trkey = KEY_BLUE;
				break;
			case '0':
				trkey = RC_0;
				break;
			case '1':
				trkey = RC_1;
				break;
			case '2':
				trkey = RC_2;
				break;
			case '3':
				trkey = RC_3;
				break;
			case '4':
				trkey = RC_4;
				break;
			case '5':
				trkey = RC_5;
				break;
			case '6':
				trkey = RC_6;
				break;
			case '7':
				trkey = RC_7;
				break;
			case '8':
				trkey = RC_8;
				break;
			case '9':
				trkey = RC_9;
				break;
			case '+':
				trkey = RC_plus;
				break;
			case '-':
				trkey = RC_minus;
				break;
			case 'a':
				trkey = KEY_A;
				break;
			case 'u':
				trkey = KEY_U;
				break;
			case '/':
				trkey = KEY_SLASH;
				break;
			case '\\':
				trkey = KEY_BACKSLASH;
				break;
			default:
				trkey = RC_nokey;
			}
			if (trkey != RC_nokey)
			{
				*msg = trkey;
				*data = 0; /* <- button pressed */
				return;
			}
		}
#else
/*
                if(FD_ISSET(fd_keyb, &rfds))
                {
                        char key = 0;
                        read(fd_keyb, &key, sizeof(key));
                        printf("keyboard: %d\n", rc_key);
                }
*/
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

		if(FD_ISSET(fd_event, &rfds))
		{
			//printf("[neutrino] event - accept!\n");
			socklen_t          clilen;
			struct sockaddr_in cliaddr;
			clilen = sizeof(cliaddr);
			int fd_eventclient = accept(fd_event, (struct sockaddr *) &cliaddr, &clilen);

			*msg = RC_nokey;
			//printf("[neutrino] network event - read!\n");
			CEventServer::eventHead emsg;
			int read_bytes= recv(fd_eventclient, &emsg, sizeof(emsg), MSG_WAITALL);
			//printf("[neutrino] event read %d bytes - following %d bytes\n", read_bytes, emsg.dataSize );
			if ( read_bytes == sizeof(emsg) )
			{
				bool dont_delete_p = false;

				unsigned char* p;
				p= new unsigned char[ emsg.dataSize + 1 ];
				if ( p!=NULL )
			 	{
			 		read_bytes= recv(fd_eventclient, p, emsg.dataSize, MSG_WAITALL);
			 		//printf("[neutrino] eventbody read %d bytes - initiator %x\n", read_bytes, emsg.initiatorID );

					if ( emsg.initiatorID == CEventServer::INITID_CONTROLD )
					{
						switch(emsg.eventID)
						{
							case CControldClient::EVT_VOLUMECHANGED :
									*msg = NeutrinoMessages::EVT_VOLCHANGED;
									*data = 0;
								break;
							case CControldClient::EVT_MUTECHANGED :
									*msg = NeutrinoMessages::EVT_MUTECHANGED;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case CControldClient::EVT_VCRCHANGED :
									*msg = NeutrinoMessages::EVT_VCRCHANGED;
									*data = *(int*) p;
								break;
							case CControldClient::EVT_MODECHANGED :
									*msg = NeutrinoMessages::EVT_MODECHANGED;
									*data = *(int*) p;
								break;
							default:
								printf("[neutrino] event INITID_CONTROLD - unknown eventID 0x%x\n",  emsg.eventID );
						}
					}
					else if ( emsg.initiatorID == CEventServer::INITID_HTTPD )
					{
						switch(emsg.eventID)
						{
							case NeutrinoMessages::SHUTDOWN :
									*msg = NeutrinoMessages::SHUTDOWN;
									*data = 0;
								break;
							case NeutrinoMessages::EVT_POPUP :
									*msg = NeutrinoMessages::EVT_POPUP;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case NeutrinoMessages::EVT_EXTMSG :
									*msg = NeutrinoMessages::EVT_EXTMSG;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case NeutrinoMessages::CHANGEMODE :	// Change
									*msg = NeutrinoMessages::CHANGEMODE;
									*data = *(unsigned*) p;
								break;
							case NeutrinoMessages::STANDBY_TOGGLE :
									*msg = NeutrinoMessages::STANDBY_TOGGLE;
									*data = 0;
								break;
							case NeutrinoMessages::STANDBY_ON :
									*msg = NeutrinoMessages::STANDBY_ON;
									*data = 0;
								break;
							case NeutrinoMessages::STANDBY_OFF :
									*msg = NeutrinoMessages::STANDBY_OFF;
									*data = 0;
								break;
							case NeutrinoMessages::ESOUND_ON :
									*msg = NeutrinoMessages::ESOUND_ON;
									*data = 0;
								break;
							case NeutrinoMessages::ESOUND_OFF :
									*msg = NeutrinoMessages::ESOUND_OFF;
									*data = 0;
								break;
							case NeutrinoMessages::EVT_START_PLUGIN :
									*msg = NeutrinoMessages::EVT_START_PLUGIN;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case NeutrinoMessages::LOCK_RC :
									*msg = NeutrinoMessages::LOCK_RC;
									*data = 0;
								break;
							case NeutrinoMessages::UNLOCK_RC :
									*msg = NeutrinoMessages::UNLOCK_RC;
									*data = 0;
								break;
							default:
								printf("[neutrino] event INITID_HTTPD - unknown eventID 0x%x\n",  emsg.eventID );
						}
					}
					else if ( emsg.initiatorID == CEventServer::INITID_SECTIONSD )
			 		{
			 			//printf("[neutrino] event - from SECTIONSD %x %x\n", emsg.eventID, *(unsigned*) p);
						switch(emsg.eventID)
						{
							case CSectionsdClient::EVT_TIMESET:
							{
								gettimeofday(&tv, NULL);
								long long timeOld = tv.tv_usec + tv.tv_sec * 1000000LL;

								time_t dvbtime = *((time_t*)p);

								if (dvbtime) {
									printf("[neutrino] timeset event. ");
									time_t difftime = dvbtime - tv.tv_sec;
									if (abs(difftime) > 120)
									{
										printf("difference is %ld s, stepping...\n", difftime);
										tv.tv_sec = dvbtime;
										tv.tv_usec = 0;
										if (settimeofday(&tv, NULL) < 0)
											perror("[neutrino] settimeofday");
									}
									else if (difftime != 0)
									{
										struct timeval oldd;
										tv.tv_sec = difftime;
										tv.tv_usec = 0;
										if (adjtime(&tv, &oldd))
											perror("adjtime");
										long long t = oldd.tv_sec * 1000000LL + oldd.tv_usec;
										printf("difference is %ld s, using adjtime(). oldd: %lld us\n", difftime, t);
									}
									else
										printf("difference is 0 s, nothing to do...\n");
								}

								gettimeofday(&tv, NULL);
								long long timeNew = tv.tv_usec + tv.tv_sec * 1000000LL;

								delete [] p;
								p = new unsigned char[sizeof(long long)];
								*(long long*) p = timeNew - timeOld;

								if ((long long)last_keypress > *(long long*)p)
									last_keypress += *(long long *)p;

								// Timer anpassen
								for (std::vector<timer>::iterator e = timers.begin(); e != timers.end(); ++e)
									if (e->correct_time)
										e->times_out += *(long long*) p;

								*msg  = NeutrinoMessages::EVT_TIMESET;
								*data = (neutrino_msg_data_t) p;
								dont_delete_p = true;
								break;
							}
							case CSectionsdClient::EVT_GOT_CN_EPG:
									*msg          = NeutrinoMessages::EVT_CURRENTNEXT_EPG;
									*data         = (neutrino_msg_data_t) p;
									dont_delete_p = true;
								break;
							case CSectionsdClient::EVT_SERVICES_UPDATE:
									*msg          = NeutrinoMessages::EVT_SERVICES_UPD;
									*data         = 0;
								break;
							case CSectionsdClient::EVT_WRITE_SI_FINISHED:
									*msg          = NeutrinoMessages::EVT_SI_FINISHED;
									*data         = 0;
								break;
							case CSectionsdClient::EVT_BOUQUETS_UPDATE:
								break;
							default:
								printf("[neutrino] event INITID_SECTIONSD - unknown eventID 0x%x\n",  emsg.eventID );
						}
			 		}
			 		else if ( emsg.initiatorID == CEventServer::INITID_ZAPIT )
			 		{
			 			//printf("[neutrino] event - from ZAPIT %x %x\n", emsg.eventID, *(unsigned*) p);
						switch(emsg.eventID)
						{
						case CZapitClient::EVT_RECORDMODE_ACTIVATED:
							*msg  = NeutrinoMessages::EVT_RECORDMODE;
							*data = true;
							break;
						case CZapitClient::EVT_RECORDMODE_DEACTIVATED:
							*msg  = NeutrinoMessages::EVT_RECORDMODE;
							*data = false;
							break;
						case CZapitClient::EVT_ZAP_COMPLETE:
							*msg = NeutrinoMessages::EVT_ZAP_COMPLETE;
							break;
						case CZapitClient::EVT_ZAP_FAILED:
							*msg = NeutrinoMessages::EVT_ZAP_FAILED;
							break;
						case CZapitClient::EVT_ZAP_SUB_FAILED:
							*msg = NeutrinoMessages::EVT_ZAP_SUB_FAILED;
							break;
						case CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD:
							*msg = NeutrinoMessages::EVT_ZAP_ISNVOD;
							break;
						case CZapitClient::EVT_ZAP_SUB_COMPLETE:
							*msg = NeutrinoMessages::EVT_ZAP_SUB_COMPLETE;
							break;
						case CZapitClient::EVT_SCAN_COMPLETE:
							*msg  = NeutrinoMessages::EVT_SCAN_COMPLETE;
							*data = 0;
							break;
						case CZapitClient::EVT_SCAN_NUM_TRANSPONDERS:
							*msg  = NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
							*msg  = NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_REPORT_FREQUENCY:
							*msg = NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_FOUND_A_CHAN:
							*msg = NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN;
							break;
						case CZapitClient::EVT_SCAN_SERVICENAME:
							*msg = NeutrinoMessages::EVT_SCAN_SERVICENAME;
							break;
						case CZapitClient::EVT_SCAN_FOUND_TV_CHAN:
							*msg  = NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN:
							*msg  = NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_FOUND_DATA_CHAN:
							*msg  = NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_REPORT_FREQUENCYP:
							*msg  = NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_NUM_CHANNELS:
							*msg = NeutrinoMessages::EVT_SCAN_NUM_CHANNELS;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_PROVIDER:
							*msg = NeutrinoMessages::EVT_SCAN_PROVIDER;
							break;
						case CZapitClient::EVT_SCAN_SATELLITE:
							*msg = NeutrinoMessages::EVT_SCAN_SATELLITE;
							break;
						case CZapitClient::EVT_BOUQUETS_CHANGED:
							*msg  = NeutrinoMessages::EVT_BOUQUETSCHANGED;
							*data = 0;
							break;
						case CZapitClient::EVT_ZAP_CA_CLEAR:
							*msg  = NeutrinoMessages::EVT_ZAP_CA_CLEAR;
							break;
						case CZapitClient::EVT_ZAP_CA_LOCK:
							*msg  = NeutrinoMessages::EVT_ZAP_CA_LOCK;
							break;
						case CZapitClient::EVT_ZAP_CA_FTA:
							*msg  = NeutrinoMessages::EVT_ZAP_CA_FTA;
							break;
						case CZapitClient::EVT_SCAN_FAILED:
							*msg  = NeutrinoMessages::EVT_SCAN_FAILED;
							*data = 0;
							break;
						case CZapitClient::EVT_ZAP_MOTOR:
							*msg  = NeutrinoMessages::EVT_ZAP_MOTOR;
							*data = *(unsigned*) p;
							break;
						default:
							printf("[neutrino] event INITID_ZAPIT - unknown eventID 0x%x\n",  emsg.eventID );
						}
						if (((*msg) >= CRCInput::RC_WithData) && ((*msg) < CRCInput::RC_WithData + 0x10000000))
						{
							*data         = (neutrino_msg_data_t) p;
							dont_delete_p = true;
						}
			 		}
			 		else if ( emsg.initiatorID == CEventServer::INITID_TIMERD )
			 		{
/*
						if (emsg.eventID==CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM)
			 			{
						}

						if (emsg.eventID==CTimerdClient::EVT_NEXTPROGRAM)
			 			{
			 				*msg = NeutrinoMessages::EVT_NEXTPROGRAM;
			 				*data = (neutrino_msg_data_t) p;
			 				dont_delete_p = true;
			 			}
*/
						switch(emsg.eventID)
						{
							case CTimerdClient::EVT_ANNOUNCE_RECORD :
									*msg = NeutrinoMessages::ANNOUNCE_RECORD;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_ANNOUNCE_ZAPTO :
									*msg = NeutrinoMessages::ANNOUNCE_ZAPTO;
									*data = 0;
								break;
							case CTimerdClient::EVT_ANNOUNCE_SHUTDOWN :
									*msg = NeutrinoMessages::ANNOUNCE_SHUTDOWN;
									*data = 0;
								break;
							case CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER :
									*msg = NeutrinoMessages::ANNOUNCE_SLEEPTIMER;
									*data = 0;
								break;
							case CTimerdClient::EVT_SLEEPTIMER :
									*msg = NeutrinoMessages::SLEEPTIMER;
									*data = 0;
								break;
							case CTimerdClient::EVT_RECORD_START :
									*msg = NeutrinoMessages::RECORD_START;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_RECORD_STOP :
									*msg = NeutrinoMessages::RECORD_STOP;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_ZAPTO :
									*msg = NeutrinoMessages::ZAPTO;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_SHUTDOWN :
									*msg = NeutrinoMessages::SHUTDOWN;
									*data = 0;
								break;
							case CTimerdClient::EVT_STANDBY_ON :
									*msg = NeutrinoMessages::STANDBY_ON;
									*data = 0;
								break;
							case CTimerdClient::EVT_STANDBY_OFF :
									*msg = NeutrinoMessages::STANDBY_OFF;
									*data = 0;
								break;
							case CTimerdClient::EVT_REMIND :
									*msg = NeutrinoMessages::REMIND;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_EXEC_PLUGIN :
									*msg = NeutrinoMessages::EVT_START_PLUGIN;
									*data = (neutrino_msg_data_t)p;
									dont_delete_p = true;
								break;

							default :
								printf("[neutrino] event INITID_TIMERD - unknown eventID 0x%x\n",  emsg.eventID );

						}
					}
					else if (emsg.initiatorID == CEventServer::INITID_NEUTRINO)
					{
						if ((emsg.eventID == NeutrinoMessages::EVT_RECORDING_ENDED) &&
						    (read_bytes == sizeof(stream2file_status2_t)))
						{
							*msg  = NeutrinoMessages::EVT_RECORDING_ENDED;
							*data = (neutrino_msg_data_t) p;
							dont_delete_p = true;
						}
					}
					else if (emsg.initiatorID == CEventServer::INITID_GENERIC_INPUT_EVENT_PROVIDER)
					{
						if (read_bytes == sizeof(int))
						{
							*msg  = *(int *)p;
							*data = emsg.eventID;
						}
					}
					else
						printf("[neutrino] event - unknown initiatorID 0x%x\n",  emsg.initiatorID);
			 		if ( !dont_delete_p )
			 		{
			 			delete [] p;
			 			p= NULL;
			 		}
			 	}
			}
			else
			{
				printf("[neutrino] event - read failed!\n");
			}

			::close(fd_eventclient);

			if ( *msg != RC_nokey )
			{
				// raus hier :)
				//printf("[neutrino] event 0x%x\n", *msg);
				return;
			}
		}

		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if ((fd_rc[i] != -1) &&
			    (FD_ISSET(fd_rc[i], &rfds)))
			{
#ifdef HAVE_TRIPLEDRAGON
				int count = 0;
				/* clear the input queue and process only the latest event
				   hack to improve the behaviour of the TD remote
				   otherwise we lose key_up events due to CRCInput::clearRCMsg() */
				while (read(fd_rc[i], &ev, sizeof(t_input_event)) == sizeof(t_input_event))
					count++;
				if (count)
#else
				if (read(fd_rc[i], &ev, sizeof(t_input_event)) == sizeof(t_input_event))
#endif
				{
					uint trkey = translate(ev.code);
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
					if (ev.code == 0xff)
					{
						if (rc_last_key != 0)
						{
							*data = 1; // compat
							*msg = rc_last_key | RC_Release;
							rc_last_key = 0;
							return;
						}
					}
#endif /* HAVE_DREAMBOX_HARDWARE || HAVE_IPBOX_HARDWARE */
#ifdef HAVE_TRIPLEDRAGON
if (trkey == RC_nokey || count > 1) fprintf(stderr, "ev.code: %04hx trkey: %04x count:%d\n", ev.code, trkey, count);
					if ((ev.code & 0xff00) == 0x8000)
					{
						*data = 1; // compat
						*msg = trkey | RC_Release;
						rc_last_key = 0; /* we use rc_last_key to differentiate "press" and "repeat" */
						return;
					}
#endif
					if (trkey != RC_nokey)
					{
#if defined(OLD_RC_API) || defined(HAVE_TRIPLEDRAGON)
						if (ev.code != 0x5cfe) // always true for TD
						{
							unsigned long long now_pressed;
							bool keyok = true;

							gettimeofday( &tv, NULL );
							now_pressed = tv.tv_usec + tv.tv_sec * 1000000ULL;
#ifndef HAVE_TRIPLEDRAGON
							//alter nokia-rc-code - lastkey löschen weil sonst z.b. nicht zweimal nacheinander ok gedrückt werden kann
							if ((ev.code & 0xff00) == 0x5c00)
								rc_last_key = 0;

							if ((ev.code & 0x8000) == 0)
#else
							if (rc_last_key != trkey)
#endif
							{	// key pressed
								*msg = trkey;
								last_keypress = now_pressed;
								rc_last_key = trkey;
								repeating = false;
								//fprintf(stderr, "pressed ");
								return;
							}
							else	// repeat...
							{
								if (repeating || (now_pressed > last_keypress + repeat_block)) // delay
								{
									//fprintf(stderr, "repeat  ");
									repeating = true; 
									if (now_pressed > last_keypress + repeat_block_generic)
									{ // rate
										*msg = trkey | RC_Repeat;
										last_keypress = now_pressed;
										return;
									}
								}
							}
						}
#else /* OLD_RC_API */
						if (ev.type == EV_KEY)
						{
							unsigned long long evtime = ev.time.tv_sec * 1000000ULL + ev.time.tv_usec;
							//fprintf(stderr, "evtime: %llu ", evtime);
							*data = 0;
							switch (ev.value)
							{
							case 0x01:	// key pressed
								*msg = trkey;
								if (repeat_kernel)
									break;
								last_keypress = evtime;
								repeating = false;
								//fprintf(stderr, "pressed ");
								break;
							case 0x02:	// key repeat
								*msg = trkey | RC_Repeat;
								if (repeat_kernel)
									break;
								// unfortunately, the old dbox remote control driver did no rate control
								if (repeating || (evtime > last_keypress + repeat_block)) // delay
								{
									//fprintf(stderr, "repeat  ");
									repeating = true;
									if (evtime > last_keypress + repeat_block_generic) // rate
										last_keypress = evtime;
								}
								else
									*msg = RC_ignore; // KEY_RESERVED
								break;
							case 0x00:	// key released
								*data = 1; // compat
								*msg = trkey | RC_Release;
								break;
							default:
								fprintf(stderr, "%s:%d: unknown ev.value: 0x%08x\n",__FUNCTION__, __LINE__, ev.value);
								*msg = RC_ignore; // KEY_RESERVED
								break;
							}
							//fprintf(stderr, "%04x %04x\n", (int)*msg, (int)*data);
							neutrino_msg_data_t the_data = 0;
							user_translate(msg, &the_data);
							//fprintf(stderr, "************ %u %s %u %u\n", *msg & KEY_MAX, keycode2keyname(*msg& KEY_MAX), *msg & (RC_Repeat | RC_Release), the_data);
							if (the_data) {
							  if (*msg == NeutrinoMessages::CHANGEMODE) {
							    *data = /*(neutrino_msg_t)*/ the_data;
							  } else {
							    // Neutrino (see neutrino.cpp) is supposed to free the
							    // memory allocated. This may or may not be working.
							    char *buf = new char[(strlen((char *)the_data)+1)*sizeof(unsigned char)];
							    strcpy((char *)buf, (char *) the_data);
							    *data =(neutrino_msg_t) buf;
							  }
							}
							if (*msg != RC_ignore) // no need to push events that need to be ignored anyway.
								return;
						}
#endif /* OLD_RC_API */
					}
				}
			}
		} 

		if(FD_ISSET(fd_pipe_low_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_low_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			// printf("got event from low-pri pipe %x %x\n", *msg, *data );

			return;
		}

        if ( InitialTimeout == 0 )
		{
			//nicht warten wenn kein key da ist
			*msg = RC_timeout;
			*data = 0;
			return;
		}
		else
		{
			//timeout neu kalkulieren
			gettimeofday( &tv, NULL );
			long long getKeyNow = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);
			long long diff = (getKeyNow - getKeyBegin);
			if( Timeout <= (unsigned long long) diff )
			{
				*msg = RC_timeout;
				*data = 0;
				return;
			}
			else
				Timeout -= diff;
		}
	}
}

/* helper function. Machines which can set this in the driver can iplement it here. */
void CRCInput::setRepeat(unsigned int delay,unsigned int period)
{
	repeat_block = delay * 1000ULL;
	repeat_block_generic = period * 1000ULL;
	repeat_kernel = false;

#ifdef HAVE_TRIPLEDRAGON
	int avsfd = ::open("/dev/stb/tdsystem", O_RDWR);
	struct BIOS_CONFIG_AREA bca;
	unsigned short rc_addr = 0xff;
	if (ioctl(avsfd, IOC_AVS_GET_LOADERCONFIG, &bca) == 0)
	{
		rc_addr = bca.ir_adrs;
		if (ioctl(fd_rc[0], IOC_IR_SET_ADDRESS, rc_addr) < 0)
			printf("[neutrino] %s: IOC_IR_SET_ADDRESS %d failed: %m\n", __FUNCTION__, rc_addr);
	}
	/* short delay in the driver improves responsiveness and reduces spurious
	   "key up" events during zapping */
	ioctl(fd_rc[0], IOC_IR_SET_DELAY, 1);
	printf("[neutrino] %s: delay=%d period=%d rc_addr=%d\n", __FUNCTION__, delay, period, rc_addr);
#else
	int ret;
	struct my_repeat {
		unsigned int delay;	// in ms
		unsigned int period;	// in ms
	};

	struct my_repeat n;

	n.delay = delay;
	n.period = period;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			if ((ret = ioctl(fd_rc[i], EVIOCSREP, &n)) < 0)
				printf("[neutrino] can not use input repeat on fd_rc[%d]: %d (%m) \n", i, errno);
			else
				repeat_kernel = true;
		}
	}
	printf("[neutrino] %s: delay=%d period=%d use kernel-repeat: %s\n", __FUNCTION__, delay, period, repeat_kernel?"yes":"no");
#endif
}

void CRCInput::postMsg(const neutrino_msg_t msg, const neutrino_msg_data_t data, const bool Priority)
{
//	printf("postMsg %x %x %d\n", msg, data, Priority );

	struct event buf;
	buf.msg  = msg;
	buf.data = data;

	if (Priority)
		write(fd_pipe_high_priority[1], &buf, sizeof(buf));
	else
		write(fd_pipe_low_priority[1], &buf, sizeof(buf));
}


void CRCInput::clearRCMsg()
{
#ifndef HAVE_TRIPLEDRAGON
	t_input_event ev;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			while (read(fd_rc[i], &ev, sizeof(t_input_event)) == sizeof(t_input_event))
				;
		}
	}
#endif
}

/**************************************************************************
*       isNumeric - test if key is 0..9
*
**************************************************************************/
bool CRCInput::isNumeric(const neutrino_msg_t key)
{
	return ((key == RC_0) || ((key >= RC_1) && (key <= RC_9)));
}

/**************************************************************************
*       getNumericValue - return numeric value of the key or -1
*
**************************************************************************/
int CRCInput::getNumericValue(const neutrino_msg_t key)
{
	return ((key == RC_0) ? (int)0 : (((key >= RC_1) && (key <= RC_9)) ? (int)(key - RC_1 + 1) : (int)-1));
}

/**************************************************************************
*       convertDigitToKey - return key representing digit or RC_nokey
*
**************************************************************************/
static const unsigned int digit_to_key[10] = {CRCInput::RC_0, CRCInput::RC_1, CRCInput::RC_2, CRCInput::RC_3, CRCInput::RC_4, CRCInput::RC_5, CRCInput::RC_6, CRCInput::RC_7, CRCInput::RC_8, CRCInput::RC_9};

unsigned int CRCInput::convertDigitToKey(const unsigned int digit)
{
	return (digit < 10) ? digit_to_key[digit] : RC_nokey;
}

/**************************************************************************
*       getUnicodeValue - return unicode value of the key or -1
*
**************************************************************************/
#define UNICODE_VALUE_SIZE 58
static const int unicode_value[UNICODE_VALUE_SIZE] = {-1 , -1 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', -1 , -1 ,
						      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', -1 , -1 , 'A', 'S',
						      'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', -1 /* FIXME */, -1 /* FIXME */, -1 , '\\', 'Z', 'X', 'C', 'V',
						      'B', 'N', 'M', ',', '.', '/', -1, -1, -1, ' '};

int CRCInput::getUnicodeValue(const neutrino_msg_t key)
{
	if (key < UNICODE_VALUE_SIZE)
		return unicode_value[key];
	else
		return -1;
}

/**************************************************************************
*       transforms the rc-key to const char *
*
**************************************************************************/
const char * CRCInput::getSpecialKeyName(const unsigned int key)
{
#ifdef UGLY_OLD_CODE
	switch(key)
	{
			case RC_standby:
			return "standby";
			case RC_home:
			return "home";
			case RC_setup:
			return "setup";
			case RC_red:
			return "red button";
			case RC_green:
			return "green button";
			case RC_yellow:
			return "yellow button";
			case RC_blue:
			return "blue button";
			case RC_page_up:
			return "page up";
			case RC_page_down:
			return "page down";
			case RC_up:
			return "cursor up";
			case RC_down:
			return "cursor down";
			case RC_left:
			return "cursor left";
			case RC_right:
			return "cursor right";
			case RC_ok:
			return "ok";
			case RC_plus:
			return "vol. inc";
			case RC_minus:
			return "vol. dec";
			case RC_spkr:
			return "mute";
			case RC_help:
			return "help";
			case RC_tv:
				return "tv";
			case RC_radio:
				return "radio";
			case RC_audio:
				return "audio";
			case RC_video:
				return "video";
			case RC_text:
				return "txt";

			// Keyboard keys
			case RC_esc:
			  return "escape";
			case RC_hyphen:
			  return "hyphen";
			case RC_equal:
			  return "equal";
			case RC_backspace:
			  return "backspace";
			case RC_tab:
			  return "tab";
			case RC_leftbrace:
			  return "leftbrace";
			case RC_rightbrace:
			  return "rightbrace";
			case RC_enter:
			  return "enter";
			case RC_leftctrl:
			  return "leftctrl";
			case RC_semicolon:
			  return "semicolon";
			case RC_apostrophe:
			  return "apostrophe";
			case RC_grave:
			  return "grave";
			case RC_leftshift:
			  return "leftshift";
			case RC_backslash:
			  return "backslash";
			case RC_comma:
			  return "comma";
			case RC_dot:
			  return "dot";
			case RC_slash:
			  return "slash";
			case RC_rightshift:
			  return "rightshift";
			case RC_kpasterisk:
			    return "kpasterisk";
			case RC_leftalt:
			  return "leftalt";
			case RC_space:
			  return "space";
			case RC_capslock:
			  return "capslock";
			case RC_f1:
			  return "f1";
			case RC_f2:
			  return "f2";
			case RC_f3:
			  return "f3";
			case RC_f4:
			  return "f4";
			case RC_f5:
			  return "f5";
			case RC_f6:
			  return "f6";
			case RC_f7:
			  return "f7";
			case RC_f8:
			  return "f8";
			case RC_f9:
			  return "f9";
			case RC_f10:
			  return "f10";
			case RC_numlock:
			  return "numlock";
			case RC_scrolllock:
			  return "scrolllock";
			case RC_kp7:
			  return "kp7";
			case RC_kp8:
			  return "kp8";
			case RC_kp9:
			  return "kp9";
			case RC_kpminus:
			  return "kpminus";
			case RC_kp4:
			  return "kp4";
			case RC_kp5:
			  return "kp5";
			case RC_kp6:
			  return "kp6";
			case RC_kpplus:
			  return "kpplus";
			case RC_kp1:
			  return "kp1";
			case RC_kp2:
			  return "kp2";
			case RC_kp3:
			  return "kp3";
			case RC_kp0:
			  return "kp0";
			case RC_kpdot:
			  return "kpdot";
			case RC_102nd:
			  return "102nd";
			case RC_kpenter:
			  return "kpenter";
			case RC_kpslash:
			  return "kpslash";
			case RC_sysrq:
			  return "sysrq";
			case RC_rightalt:
			  return "rightalt";
			case RC_end:
			  return "end";
			case RC_insert:
			  return "insert";
			case RC_delete:
			  return "delete";
			case RC_pause:
			  return "pause";
			case RC_leftmeta:
			  return "leftmeta";
			case RC_rightmeta:
			  return "rightmeta";
			case RC_btn_left:
			  return "btn_left";
			case RC_btn_right:
			  return "btn_right";
			case RC_top_left:
			  return "top_left";
			case RC_top_right:
			  return "top_right";
			case RC_bottom_left:
			  return "bottom_left";
			case RC_bottom_right:
			  return "bottom_right";

			case RC_timeout:
			return "timeout";
			case RC_nokey:
			return "none";

			default:
			return "unknown";
	}
#else
	return strlen(CRCInput::keycode2keyname(key)) ?  CRCInput::keycode2keyname(key) : "none"; 
#endif
}

std::string CRCInput::getKeyName(const unsigned int key)
{
	int uc_value = getUnicodeValue(key);
	if (uc_value == -1)
		return getSpecialKeyName(key);
	else
	{
		char tmp[2];
		tmp[0] = uc_value;
		tmp[1] = 0;
		return std::string(tmp);
	}
}

/**************************************************************************
*	transforms the rc-key to generic - internal use only!
*
**************************************************************************/
int CRCInput::translate(int code)
{
#ifdef HAVE_TRIPLEDRAGON
	switch (code&0xFF)
	{
		case 0x01: return RC_standby;
		case 0x02: return RC_1;
		case 0x03: return RC_2;
		case 0x04: return RC_3;
		case 0x05: return RC_4;
		case 0x06: return RC_5;
		case 0x07: return RC_6;
		case 0x08: return RC_timer;
		case 0x09: return RC_7;
		case 0x0a: return RC_8;
		case 0x0b: return RC_9;
		case 0x0c: return RC_zoomin;
		case 0x0d: return RC_fav;
		case 0x0e: return RC_0;
		case 0x0f: return RC_zoomoff;	// red hand
		case 0x10: return RC_zoomout;
		case 0x11: return RC_spkr;	// MUTE
		case 0x12: return RC_setup;	// menu
		case 0x13: return RC_epg;
		case 0x14: return RC_help;	// INFO
		case 0x15: return RC_home;	// EXIT
		case 0x16: return RC_page_down;	// vv
		case 0x17: return RC_page_up;	// ^^
		case 0x18: return RC_up;
		case 0x19: return RC_left;
		case 0x1a: return RC_ok;
		case 0x1b: return RC_right;
		case 0x1c: return RC_down;
		case 0x1d: return RC_minus;
		case 0x1e: return RC_plus;
		case 0x1f: return RC_red;
		case 0x20: return RC_green;
		case 0x21: return RC_yellow;
		case 0x22: return RC_blue;
		case 0x23: return RC_tv;	// TV/RADIO
		case 0x24: return RC_video;	// MP3/PVR
		case 0x25: return RC_audio;	// CD/DVD
		case 0x26: return RC_aux;	// AUX
		case 0x27: return RC_text;	// [=]
		case 0x28: return RC_tttv;	// [ /=]
		case 0x29: return RC_ttzoom;	// [=x=]
		case 0x2a: return RC_ttreveal;	// [=?]
		case 0x2b: return RC_back;	// <<
		case 0x2c: return RC_stop;	// X
		case 0x2d: return RC_playpause;	// ||>
		case 0x2e: return RC_forward;	// >>
		case 0x2f: return RC_prev;	// |<<
		case 0x30: return RC_eject;
		case 0x31: return RC_record;
		case 0x32: return RC_next;	// >>|
	}
	return RC_nokey;
#endif
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
	switch (code&0xFF)
	{
		case 0x00: return RC_0;
		case 0x01: return RC_1;
		case 0x02: return RC_2;
		case 0x03: return RC_3;
		case 0x04: return RC_4;
		case 0x05: return RC_5;
		case 0x06: return RC_6;
		case 0x07: return RC_7;
		case 0x08: return RC_8;
		case 0x09: return RC_9;
		case 0x0a: return RC_plus;
		case 0x0b: return RC_minus;
		case 0x0d: return RC_page_up;
		case 0x0e: return RC_page_down;
		case 0x0f: return RC_standby;
		case 0x20: return RC_setup;
		case 0x21: return RC_up;
		case 0x22: return RC_down;
		case 0x23: return RC_left;
		case 0x24: return RC_right;
		case 0x25: return RC_ok;
		case 0x26: return RC_audio;
		case 0x27: return RC_video;
		case 0x28: return RC_help;
		case 0x40: return RC_red;
		case 0x41: return RC_green;
		case 0x42: return RC_yellow;
		case 0x43: return RC_blue;
		case 0x45: return RC_text;
		case 0x53: return RC_radio;
#ifdef BOXMODEL_DM500
		case 0x0c: return RC_spkr;	// MUTE key
		case 0x44: return RC_tv;	// TV   key
		case 0x50: return RC_plus;	// ">"	key
		case 0x51: return RC_minus;	// "<"  key
		case 0x52: return RC_help;	// HELP key
		case 0x54: return RC_home;	// EXIT key
#else
		case 0x0c: return RC_tv;
		case 0x44: return RC_spkr;
		case 0x50: return RC_prev;
		case 0x51: return RC_next;
		case 0x52: return RC_home;
		case 0x54: return RC_help;
#endif
	}
	return RC_nokey;
#else
#ifdef OLD_RC_API
	if ((code&0xFF00)==0x5C00)
	{
		switch (code&0xFF)
		{
		case 0x0C:
			return RC_standby;
		case 0x20:
			return RC_home;
		case 0x27:
			return RC_setup;
		case 0x00:
			return RC_0;
		case 0x01:
			return RC_1;
		case 0x02:
			return RC_2;
		case 0x03:
			return RC_3;
		case 0x04:
			return RC_4;
		case 0x05:
			return RC_5;
		case 0x06:
			return RC_6;
		case 0x07:
			return RC_7;
		case 0x08:
			return RC_8;
		case 0x09:
			return RC_9;
		case 0x3B:
			return RC_blue;
		case 0x52:
			return RC_yellow;
		case 0x55:
			return RC_green;
		case 0x2D:
			return RC_red;
		case 0x54:
			return RC_page_up;
		case 0x53:
			return RC_page_down;
		case 0x0E:
			return RC_up;
		case 0x0F:
			return RC_down;
		case 0x2F:
			return RC_left;
		case 0x2E:
			return RC_right;
		case 0x30:
			return RC_ok;
		case 0x16:
			return RC_plus;
		case 0x17:
			return RC_minus;
		case 0x28:
			return RC_spkr;
		case 0x82:
			return RC_help;
		default:
			return RC_nokey;
		}
	}
	else if ((code & 0xFF00) == 0xFF00)
	{
		switch (code & 0xFF)
		{
		case 0x12:
		case 0x9d:
			return RC_standby;
		case 0x48:
		case 0xab:
			return RC_down;
		case 0x24:
		case 0xc7:
			return RC_up;
		case 0x20:
		case 0x40:
		case 0xaf:
		case 0xcf:
			return RC_nokey;
		case 0x10:
		case 0x9f:
			return RC_standby_release;
		}
	}
	else if (!(code&0x00))
	{
		static const uint translation[0x21 + 1] = 
			{ RC_0           , RC_1   , RC_2      , RC_3        , RC_4    , RC_5    , RC_6      , RC_7       , RC_8        , RC_9          ,
			  RC_right       , RC_left, RC_up     , RC_down     , RC_ok   , RC_spkr , RC_standby, RC_green   , RC_yellow   , RC_red        ,
			  RC_blue        , RC_plus, RC_minus  , RC_help     , RC_setup, RC_nokey, RC_nokey  , RC_top_left, RC_top_right, RC_bottom_left,
			  RC_bottom_right, RC_home, RC_page_up, RC_page_down};
		if ((code & 0x3F) <= 0x21)
			return translation[code & 0x3F];
		else
			return RC_nokey;
	}
	
	return RC_nokey;
#else /* OLD_RC_API */
	if ((code >= 0) && (code <= KEY_MAX))
		return code;
	else
		return RC_nokey;
#endif /* OLD_RC_API */
#endif /* HAVE_DREAMBOX_HARDWARE */
}

// This function is called with input message pointed to by the first
// argument. This is looked up in the translate table, acted upon in
// some cases, otherwise return the possibly changed message. The
// second argument may, through the pointer, return data, like plugin
// name.
void CRCInput::user_translate(neutrino_msg_t *msg, neutrino_msg_data_t *data)
{
	*data = 0;
	if (*msg == RC_ignore || *msg == RC_nokey)
		return;

	uint modifier = (*msg & (RC_Repeat | RC_Release)) >> 10;
	neutrino_msg_t incode = *msg & KEY_MAX;
	//printf("[rcinput] %u %u\n", incode, modint2ch(modifier));

	if (!CFrameBuffer::getInstance()->getActiveReally() && no_neutrinoevents_when_vc)
	{
		if (debug_user_translate)
			printf("[rcinput] key %s[%c] rejected (virtual console open)\n",
			       keycode2keyname(incode), modint2ch(modifier));
		*msg = RC_ignore;//RC_nokey;
		return;
	}

	if (user_translate_table[modifier].find(incode) != user_translate_table[modifier].end())
	{
		*msg = user_translate_table[modifier][incode];

		if (user_translate_data[modifier].find(incode) == user_translate_data[modifier].end()) {
			*data = 0;
			if (debug_user_translate)
				printf("[rcinput] user_translate: %s[%c] -> %s\n", keycode2keyname(incode), modint2ch(modifier),
				       keycode2keyname(*msg));
		}
		else
		{
			*data = user_translate_data[modifier][incode];
			if (debug_user_translate)
				printf("[rcinput] user_translate: %s[%c] -> %s(%s)\n",
				       keycode2keyname(incode), modint2ch(modifier),
				       keycode2keyname(*msg), (const char*) *data);
		}
	}
	else
	{
		data = 0;
		if (debug_user_translate)
			printf("[rcinput] user_translate: %s[%c] NOT translated\n",
			       keycode2keyname(incode&RC_MaxRC), modint2ch(modifier));
	}

	if (*msg == RELOAD_CONF)
	{
		load_conf(false);
		*msg = RC_ignore;//nokey;
	}
	else if (*msg == SYSTEM)
	{
		const char *cmd = (const char *) user_translate_data[modifier][incode];
		if (cmd)
		{
			if (debug_user_translate)
				printf("[rcinput] user_translate system(%s), now trying to execute...\n", cmd);
			int result = system(cmd);
			if (debug_user_translate)
				printf(".... return code: %d\n", result);
		}
		else
			if (debug_user_translate)
				printf("[rcinput] user_translate system NOT defined (internal error?)\n");    
		*msg = RC_ignore;//RC_nokey;
	}
	else if (*msg == MODE_TV)
	{
		*data = 1;			// CNeutrinoApp::mode_tv (private(!!) in neutrino.h)
		*msg = NeutrinoMessages::CHANGEMODE;
	}
	else if (*msg == MODE_RADIO)
	{
		*data = 2;			// CNeutrinoApp::mode_radio (private(!!) neutrino.h)
		*msg = NeutrinoMessages::CHANGEMODE;
	} /* else do nothing */
}
