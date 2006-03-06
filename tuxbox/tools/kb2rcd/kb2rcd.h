/******************************************************************************
 *                        <<< Keyboard 2 RemoteControl daemon >>>
 *                (c) Robert "robspr1" Spreitzer 2006 (robert.spreitzer@inode.at)
 *-----------------------------------------------------------------------------
 * $Log: kb2rcd.h,v $
 * Revision 0.11  2006/03/06 21:09:46  robspr1
 * - change to kb2rcd.conf and change mouse behaviour
 *
 * Revision 0.10  2006/03/05 22:39:03  robspr1
 * - add to cvs
 *
 * Revision 0.9  2006/03/05 22:50:00  robspr1
 * - change mouse-cursor behavior
 * - add debug-output switch
 *
 * Revision 0.8  2006/03/05 17:50:00  robspr1
 * - add delay between keystrokes
 *
 * Revision 0.7  2006/03/05 16:30:00  robspr1
 * - add debug_output with key_names
 *
 * Revision 0.6  2006/03/05 16:00:00  robspr1
 * - add config for mouse-cursor
 * - release button before sending a new button
 *
 * Revision 0.5  2006/03/05 15:00:00  robspr1
 * - add mouse-cursor
 *
 * Revision 0.4  2006/03/05 12:00:00  robspr1
 * - add lock-file for conversion
 *
 * Revision 0.3  2006/03/05 10:00:00  robspr1
 * - default .conf file is generated
 *
 * Revision 0.2  2006/03/03 22:00:00  robspr1
 * - change rc-read from NON-BLOCKING to BLOCKING
 * - fix writing ALT_ and SHIFT_ keys
 * - fix signal -HUP
 *
 * Revision 0.1  2006/03/03 20:00:00  robspr1
 * - first version
 *
 ******************************************************************************/

#include "config.h"

#if !defined(HAVE_DVB_API_VERSION) && defined(HAVE_OST_DMX_H)
#define HAVE_DVB_API_VERSION 1
#endif


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <linux/fb.h>
#include <zlib.h>
#include <malloc.h>
#include <linux/input.h>


#define CFGPATH "/var/tuxbox/config/"									//! config-path
#define CFGFILE "kb2rcd.conf"													//! config-file
#define PIDFILE "/tmp/kb2rcd.pid"											//! PID file
#define ACTFILE "/tmp/kb2rcd.act"											//! file to signal active conversion
#define LCKFILE "/tmp/keyboard.lck"										//! file to lock conversion

#define bool char
#define true 1
#define false 0

//----------------------------------------------------
// remote-control and keyboard

struct key{
	char *name;
	unsigned long code;
};

#define PAUSE100				0xFFFF0100
#define PAUSE250				0xFFFF0250
#define PAUSE500				0xFFFF0500
#define PAUSE1000				0xFFFF1000

static const struct key keyname[] = {
	{"KEY_0", 					KEY_0},
	{"KEY_1", 					KEY_1},
	{"KEY_2", 					KEY_2},
	{"KEY_3", 					KEY_3},
	{"KEY_4", 					KEY_4},
	{"KEY_5", 					KEY_5},
	{"KEY_6", 					KEY_6},
	{"KEY_7", 					KEY_7},
	{"KEY_8", 					KEY_8},
	{"KEY_9", 					KEY_9},
	{"KEY_RIGHT",				KEY_RIGHT},
	{"KEY_LEFT",				KEY_LEFT},
	{"KEY_UP",					KEY_UP},
	{"KEY_DOWN",				KEY_DOWN},
	{"KEY_OK",					KEY_OK},
	{"KEY_MUTE",				KEY_MUTE},
	{"KEY_POWER",				KEY_POWER},
	{"KEY_GREEN",				KEY_GREEN},
	{"KEY_YELLOW",			KEY_YELLOW},
	{"KEY_RED",					KEY_RED},
	{"KEY_BLUE",				KEY_BLUE},
	{"KEY_VOLUMEUP",		KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN",	KEY_VOLUMEDOWN},
	{"KEY_HELP",				KEY_HELP},
	{"KEY_SETUP",				KEY_SETUP},
	{"KEY_TOPLEFT",			KEY_TOPLEFT},
	{"KEY_TOPRIGHT", 		KEY_TOPRIGHT},
	{"KEY_BOTTOMLEFT",	KEY_BOTTOMLEFT},
	{"KEY_BOTTOMRIGHT",	KEY_BOTTOMRIGHT},
	{"KEY_HOME",				KEY_HOME},
	{"KEY_PAGEDOWN",		KEY_PAGEDOWN},
	{"KEY_PAGEUP",			KEY_PAGEUP},
	{"KEY_ESC",					KEY_ESC},
	{"KEY_HYPHEN",			KEY_MINUS},
	{"KEY_EQUAL",				KEY_EQUAL},
	{"KEY_BACKSPACE",		KEY_BACKSPACE},
	{"KEY_TAB",					KEY_TAB},
	{"KEY_Q",						KEY_Q},
	{"KEY_W",						KEY_W},
	{"KEY_E",						KEY_E},
	{"KEY_R",						KEY_R},
	{"KEY_T",						KEY_T},
	{"KEY_Y",						KEY_Y},
	{"KEY_U",						KEY_U},
	{"KEY_I",						KEY_I},
	{"KEY_O",						KEY_O},
	{"KEY_P",						KEY_P},
	{"KEY_LEFTBRACE",		KEY_LEFTBRACE},
	{"KEY_RIGHTBRACE",	KEY_RIGHTBRACE},
	{"KEY_ENTER",				KEY_ENTER},
	{"KEY_LEFTCTRL",		KEY_LEFTCTRL},
	{"KEY_A",						KEY_A},
	{"KEY_S",						KEY_S},
	{"KEY_D",						KEY_D},
	{"KEY_F",						KEY_F},
	{"KEY_G",						KEY_G},
	{"KEY_H",						KEY_H},
	{"KEY_J",						KEY_J},
	{"KEY_K",						KEY_K},
	{"KEY_L",						KEY_L},
	{"KEY_SEMICOLON",		KEY_SEMICOLON},
	{"KEY_APOSTROPHE",	KEY_APOSTROPHE},
	{"KEY_GRAVE",				KEY_GRAVE},
	{"KEY_LEFTSHIFT",		KEY_LEFTSHIFT},
	{"KEY_BACKSLASH",		KEY_BACKSLASH},
	{"KEY_Z",						KEY_Z},
	{"KEY_X",						KEY_X},
	{"KEY_C",						KEY_C},
	{"KEY_V",						KEY_V},
	{"KEY_B",						KEY_B},
	{"KEY_N",						KEY_N},
	{"KEY_M",						KEY_M},
	{"KEY_COMMA",				KEY_COMMA}, 	
	{"KEY_DOT",					KEY_DOT}, 		
	{"KEY_SLASH",				KEY_SLASH}, 	
	{"KEY_RIGHTSHIFT",	KEY_RIGHTSHIFT}, 	
	{"KEY_KPASTERISK",	KEY_KPASTERISK}, 	
	{"KEY_LEFTALT",			KEY_LEFTALT}, 	
	{"KEY_SPACE",				KEY_SPACE}, 	
	{"KEY_CAPSLOCK",		KEY_CAPSLOCK}, 
	{"KEY_F1",					KEY_F1}, 	
	{"KEY_F2",					KEY_F2}, 	
	{"KEY_F3",					KEY_F3}, 	
	{"KEY_F4",					KEY_F4}, 	
	{"KEY_F5",					KEY_F5}, 	
	{"KEY_F6",					KEY_F6}, 	
	{"KEY_F7",					KEY_F7}, 	
	{"KEY_F8",					KEY_F8}, 	
	{"KEY_F9",					KEY_F9}, 	
	{"KEY_F10",					KEY_F10}, 	 
	{"KEY_NUMLOCK",			KEY_NUMLOCK}, 	 
	{"KEY_SCROLLLOCK",	KEY_SCROLLLOCK}, 
	{"KEY_KP7",					KEY_KP7},	
	{"KEY_KP8",					KEY_KP8},	
	{"KEY_KP9",					KEY_KP9},		
	{"KEY_KPMINUS",			KEY_KPMINUS},		
	{"KEY_KP4",					KEY_KP4},	
	{"KEY_KP5",					KEY_KP5},	
	{"KEY_KP6",					KEY_KP6},	
	{"KEY_KPPLUS",			KEY_KPPLUS},	
	{"KEY_KP1",					KEY_KP1},		
	{"KEY_KP2",					KEY_KP2},		
	{"KEY_KP3",					KEY_KP3},		
	{"KEY_KP0",					KEY_KP0},		
	{"KEY_KPDOT",				KEY_KPDOT},		
	{"KEY_102ND",				KEY_102ND}, 	 	
	{"KEY_KPENTER",			KEY_KPENTER},	
	{"KEY_KPSLASH",			KEY_KPSLASH},	
	{"KEY_RIGHTALT",		KEY_RIGHTALT}, 
	{"KEY_END",					KEY_END}, 	 
	{"KEY_INSERT",			KEY_INSERT}, 
	{"KEY_DELETE",			KEY_DELETE}, 
	{"KEY_PAUSE",				KEY_PAUSE}, 	
	{"KEY_BTNLEFT",			BTN_LEFT}, 
	{"KEY_BTNRIGHT",		BTN_RIGHT},
	{"PAUSE100",				PAUSE100},
	{"PAUSE250",				PAUSE250},
	{"PAUSE500",				PAUSE500},
	{"PAUSE1000",				PAUSE1000},
	{"",								0xFFFFFFFF}
};

#define RC_ALT		0x01000000
#define RC_SHIFT	0x02000000

struct input_event ev;													//! input event for dBox
unsigned short rccode;													//! remote-control code

enum {	// not defined in input.h but used like that, at least in 2.4.22
	KEY_RELEASED = 0,
	KEY_PRESSED,
	KEY_AUTOREPEAT
};

#define MAX_OUT				10									//! max number of codes the represent on keyborad-key
#define MAX_CONVERT		100									//! maximum number of keystrokes to convert
#define MAX_REL				80									//! maximum relative value that makes a key

struct keyconvert{
	unsigned long in_code;
	unsigned long out_code[MAX_OUT];
};

//----------------------------------------------------
// config
int webport=80;														//! webport for using webinterface
char webuser[32] = "";										//! for using webinterface
char webpass[32] = "";										//! for using webinterface
struct keyconvert keyconv[MAX_CONVERT];		//! the key-convertions
int iCount = 0;														//! number of conversions
int iMouseCnt = 0;												//! how many mouse-counts make one key
int iMinMouse = 1;												//! minimum relative value that is not ignored
int iMaxMouse = MAX_REL;									//! maximum relative value, makes a key
int iDelay = 0;														//! delay in milli-seconds between keystrokes
	
//----------------------------------------------------
// variables
FILE *fd_pid;															//! PID file
int pid;																	//! the PID
int slog = 0;															//! logging to sys
int debug = 0;														//! debug outputs
int run = 0;															//! flag for running
int active = 0;														//! conversion is active
char timeinfo[22];												//! string for time
char versioninfo_d[12] = "?.??";					//! daemon version
time_t tt;																//! actual time

//----------------------------------------------------
// devs
int rc = 0;




