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
$Log: rc.cpp,v $
Revision 1.8  2003/01/05 02:41:53  TheDOC
lcars supports inputdev now

Revision 1.7  2002/10/20 02:03:37  TheDOC
Some fixes and stuff

Revision 1.6  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.5  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.4  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.5  2001/12/17 16:54:47  tux
Settings halb komplett

Revision 1.4  2001/12/17 14:00:41  tux
Another commit

Revision 1.3  2001/12/17 03:52:42  tux
Netzwerkfernbedienung fertig

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <stdio.h>
#include "rc.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>

rc::rc(hardware *h, settings *s)
{
	setting = s;
	rcstop = false;
	hardware_obj = h;

	fp = open("/dev/input/event0", O_RDONLY);
	if (fp < 0)
	{
		perror("Could not open input device");
		exit(1);
	}

	int rcs[NUMBER_RCS][25] =
	    {
	        {
	            RC_0,
	            RC_1,
	            RC_2,
	            RC_3,
	            RC_4,
	            RC_5,
	            RC_6,
	            RC_7,
	            RC_8,
	            RC_9,
	            RC_STANDBY,
	            RC_HOME,
	            RC_DBOX,
	            RC_RED,
	            RC_GREEN,
	            RC_YELLOW,
	            RC_BLUE,
	            RC_OK,
	            RC_VOLPLUS,
	            RC_MUTE,
	            RC_HELP,
	            RC_UP,
	            RC_DOWN,
	            RC_RIGHT,
	            RC_LEFT
	        }
	    };
	//rc_codes = rcs;
	for (int i = 0; i < NUMBER_RCS; i++)
	{
		for (int j = 0; j < 25; j++)
		{
			rc_codes[i][j] = rcs[i][j];
		}
	}
}

rc::~rc()
{
	close(fp);
}

int rc::start_thread()
{

	int status;

	pthread_mutex_init(&mutex, NULL);
	status = pthread_create( &rcThread,
	                         NULL,
	                         start_rcqueue,
	                         (void *)this );

	return status;

}

void* rc::start_rcqueue( void * this_ptr )
{
	rc *r = (rc *) this_ptr;

	while(1)
	{
		if (!r->rcstop)
			pthread_mutex_unlock( &r->blockingmutex );
		else
			sleep(1);
		pthread_mutex_lock( &r->blockingmutex );

		r->key = r->read_from_rc2();
		//std::cout << "Key: " << r->key << std::endl;
	}
	return 0;
}

int rc::parseKey(std::string key)
{
	if (key == "1")
	{
		return RC_1;
	}
	else if (key == "2")
	{
		return RC_2;
	}
	else if (key == "3")
	{
		return RC_3;
	}
	else if (key == "4")
	{
		return RC_4;
	}
	else if (key == "5")
	{
		return RC_5;
	}
	else if (key == "6")
	{
		return RC_6;
	}
	else if (key == "7")
	{
		return RC_7;
	}
	else if (key == "8")
	{
		return RC_8;
	}
	else if (key == "9")
	{
		return RC_9;
	}
	else if (key == "0")
	{
		return RC_0;
	}
	else if (key == "STANDBY")
	{
		return RC_STANDBY;
	}
	else if (key == "HOME")
	{
		return RC_HOME;
	}
	else if (key == "DBOX")
	{
		return RC_DBOX;
	}
	else if (key == "RED")
	{
		return RC_RED;
	}
	else if (key == "GREEN")
	{
		return RC_GREEN;
	}
	else if (key == "YELLOW")
	{
		return RC_YELLOW;
	}
	else if (key == "BLUE")
	{
		return RC_BLUE;
	}
	else if (key == "OK")
	{
		return RC_OK;
	}
	else if (key == "VOLPLUS")
	{
		return RC_VOLPLUS;
	}
	else if (key == "VOLMINUS")
	{
		return RC_VOLMINUS;
	}
	else if (key == "MUTE")
	{
		return RC_MUTE;
	}
	else if (key == "HELP")
	{
		return RC_HELP;
	}
	else if (key == "UP")
	{
		return RC_UP;
	}
	else if (key == "DOWN")
	{
		return RC_DOWN;
	}
	else if (key == "RIGHT")
	{
		return RC_RIGHT;
	}
	else if (key == "LEFT")
	{
		return RC_LEFT;
	}
	return -1;
}

void rc::cheat_command(unsigned short cmd)
{
	key = cmd;
	last_read = cmd;
	//std::cout << "Command: " << cmd << std::endl;
	pthread_mutex_unlock( &blockingmutex );
	usleep(100);
	pthread_mutex_lock( &blockingmutex );
}

void rc::stoprc()
{
	rcstop = true;
	pthread_mutex_unlock( &blockingmutex );
	pthread_mutex_lock( &blockingmutex );
}

void rc::startrc()
{
	rcstop = false;
	pthread_mutex_unlock( &blockingmutex );
}

void rc::restart()
{
	close(fp);
	fp = open("/dev/input/event0", O_RDONLY);
}

unsigned short rc::get_last()
{
	return last_read;
}

unsigned short rc::read_from_rc()
{
	usleep(100);
	if (key == -1)
	{
		pthread_mutex_lock( &blockingmutex );
		pthread_mutex_unlock( &blockingmutex );
	}
	//std::cout << "KEY: " << key << std::endl;
	int returnkey = key;
	key = -1;
	return returnkey;
}

unsigned short rc::read_from_rc2()
{
	struct input_event read_code;
	int rd;

	pthread_mutex_lock( &mutex );

	do
	{
		rd = read(fp, &read_code, sizeof(struct input_event));
	} while(read_code.value != 1);

	if (rd < (int) sizeof(struct input_event))
	{
		perror("[rc.cpp] Error reading input-event");
		return 0;
	}
	last_read = read_code.code;

	pthread_mutex_unlock( &mutex );

	return read_code.code;
}

int rc::get_number()
{
	int i, codes, number = -1;

	for (i = 0; i < NUMBER_RCS; i++)
	{
		for (codes = 0; codes < 10; codes++)
		{
			if (last_read == rc_codes[i][codes])
				number = codes;
		}
	}
	return number;
}


int rc::command_available()
{
	return (key != -1);
}
