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
Revision 1.6  2003/01/05 02:41:53  TheDOC
lcars supports inputdev now

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

#define	NUMBER_RCS	1

#define	RC_1		2
#define	RC_2		3
#define	RC_3		4
#define	RC_4		5
#define	RC_5		6
#define	RC_6		7
#define	RC_7		8
#define	RC_8		9
#define	RC_9		10
#define	RC_0		11
#define	RC_STANDBY	116
#define	RC_HOME		102
#define	RC_DBOX		141
#define	RC_RED		398
#define	RC_GREEN	399
#define	RC_YELLOW	400
#define	RC_BLUE		401
#define	RC_OK		352
#define	RC_VOLPLUS	115
#define	RC_VOLMINUS	114
#define	RC_MUTE		113
#define	RC_HELP		138
#define	RC_UP		103
#define	RC_DOWN		108
#define	RC_RIGHT	106
#define	RC_LEFT		105

class rc
{
	int fp;
	unsigned short last_read;
	int rc_codes[NUMBER_RCS][25];

	pthread_t rcThread;
	pthread_mutex_t mutex;

	static void* start_rcqueue( void * );
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

	int start_thread();
	int getHandle() { return fp; }
	void restart();

	void cheat_command(unsigned short cmd);
	// Waits for the RC to receive a command and returns it
	unsigned short read_from_rc();
	unsigned short read_from_rc2();
	unsigned short get_last();
	int command_available();

	// Returns -1 if the latest command isn't a number, returns number else
	int get_number();
};

#endif
