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
$Log: rc.h,v $
Revision 1.5  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.4  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.3  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.5  2001/12/17 16:54:47  tux
Settings halb komplett

Revision 1.4  2001/12/17 14:00:41  tux
Another commit

Revision 1.3  2001/12/17 03:52:42  tux
Netzwerkfernbedienung fertig

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef RC_H
#define RC_H

#include "hardware.h"
#include "settings.h"

#define	NUMBER_RCS	2

#define	RC1_1		0x0001
#define	RC1_2		0x0002
#define	RC1_3		0x0003
#define	RC1_4		0x0004
#define	RC1_5		0x0005
#define	RC1_6		0x0006
#define	RC1_7		0x0007
#define	RC1_8		0x0008
#define	RC1_9		0x0009
#define	RC1_0		0x0000
#define	RC1_STANDBY	0x0010
#define	RC1_HOME		0x001F
#define	RC1_DBOX		0x0018
#define	RC1_RED		0x0013
#define	RC1_GREEN		0x0011
#define	RC1_YELLOW	0x0012
#define	RC1_BLUE		0x0014
#define	RC1_OK		0x000E
#define	RC1_VOLPLUS	0x0015
#define	RC1_VOLMINUS	0x0016
#define	RC1_MUTE		0x000F
#define	RC1_HELP		0x0017
#define	RC1_UP		0x000C
#define	RC1_DOWN		0x000D
#define	RC1_RIGHT		0x000A
#define	RC1_LEFT		0x000B

#define	RC2_1		0x5c01
#define	RC2_2		0x5c02
#define	RC2_3		0x5c03
#define	RC2_4		0x5c04
#define	RC2_5		0x5c05
#define	RC2_6		0x5c06
#define	RC2_7		0x5c07
#define	RC2_8		0x5c08
#define	RC2_9		0x5c09
#define	RC2_0		0x5c00
#define	RC2_STANDBY	0x5c0c
#define	RC2_HOME		0x5c20
#define	RC2_DBOX		0x5c27
#define	RC2_RED		0x5c2d
#define	RC2_GREEN		0x5c55
#define	RC2_YELLOW	0x5c52
#define	RC2_BLUE		0x5c3b
#define	RC2_OK		0x5c30
#define	RC2_VOLPLUS	0x5c16
#define	RC2_VOLMINUS	0x5c17
#define	RC2_MUTE		0x5c28
#define	RC2_HELP		0x5c82
#define	RC2_UP		0x5c0e
#define	RC2_DOWN		0x5c0f
#define	RC2_RIGHT		0x5c2e
#define	RC2_LEFT		0x5c2f
#define RC2_UPUP	0x5c54
#define RC2_DOWNDOWN 0x5c53

// 3 keys on the front of the box
#define FRONT_STANDBY 0xff9d
#define FRONT_DOWN 0xffab
#define FRONT_UP 0xffc7

class rc
{
	int fp;
	unsigned short last_read;
	int rc_codes[NUMBER_RCS][25];

	pthread_t rcThread;
	pthread_t keyboardThread;
	pthread_mutex_t mutex;

	static void* start_rcqueue( void * );
	static void* start_keyboardqueue( void * );
	settings *setting;

public:
	bool rcstop;
	pthread_mutex_t blockingmutex;
	int key;
	hardware *hardware_obj;

	rc(hardware *h, settings *s);
	~rc();

	int parseKey(std::string key);

	void stoprc();
	void startrc();

	int start_thread(bool withoutkeyboard = false);
	int getHandle() { return fp; }
	void restart();

	void cheat_command(unsigned short cmd);
	// Waits for the RC to receive a command and returns it
	unsigned short read_from_rc();
	unsigned short read_from_rc2();
	unsigned short read_from_rc_raw();
	unsigned short get_last();
	unsigned short convert_code(unsigned short rc);
	int command_available();

	int old_to_new(int read_code);

	// Returns -1 if the latest command isn't a number, returns number else
	int get_number();
};

#endif
