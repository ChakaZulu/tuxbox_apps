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
#include "rc.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream.h>

rc::rc(hardware *h)
{
	hardware_obj = h;

	fp = open("/dev/dbox/rc0", O_RDONLY);
	
	int rcs[NUMBER_RCS][25] =
	{
		{
			RC1_0,
			RC1_1,
			RC1_2,
			RC1_3,
			RC1_4,
			RC1_5,
			RC1_6,
			RC1_7,
			RC1_8,
			RC1_9,
			RC1_STANDBY,
			RC1_HOME,
			RC1_DBOX,
			RC1_RED,
			RC1_GREEN,
			RC1_YELLOW,
			RC1_BLUE,
			RC1_OK,
			RC1_VOLPLUS,
			RC1_MUTE,
			RC1_HELP,
			RC1_UP,
			RC1_DOWN,
			RC1_RIGHT,
			RC1_LEFT
		},
		{
			RC2_0,
			RC2_1,
			RC2_2,
			RC2_3,
			RC2_4,
			RC2_5,
			RC2_6,
			RC2_7,
			RC2_8,
			RC2_9,
			RC2_STANDBY,
			RC2_HOME,
			RC2_DBOX,
			RC2_RED,
			RC2_GREEN,
			RC2_YELLOW,
			RC2_BLUE,
			RC2_OK,
			RC2_VOLPLUS,
			RC2_MUTE,
			RC2_HELP,
			RC2_UP,
			RC2_DOWN,
			RC2_RIGHT,
			RC2_LEFT
		}
	};
	rc_codes = rcs;
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
		sleep(1);
		if ((r->get_command_without_removing() % 0x40) == RC1_STANDBY)
			r->hardware_obj->shutdown();
	}
}


void rc::restart()
{
	close(fp);
	fp = open("/dev/dbox/rc0", O_RDONLY);
}

void rc::setRepeat(bool input)
{
	repeat = input;
}

unsigned short rc::read_from_rc_raw()
{
	unsigned short read_code = 0;

	read(fp, &read_code, 2);
 	
	last_read = read_code;
	
	return read_code;
}

unsigned short rc::get_last()
{
	return last_read;
}

unsigned short rc::convert_code(unsigned short rc)
{
	return rc % 0x40;
}

unsigned short rc::read_from_rc()
{
	unsigned short read_code = 0;

	pthread_mutex_lock( &mutex );

	if (!taken_commands.empty())
	{
		read_code = taken_commands.front();
		taken_commands.pop();
		last_read = read_code;
		pthread_mutex_unlock(&mutex);
		return read_code;
	}


	if (!repeat)
	{
		do
		{
			read(fp, &read_code, 2);
			printf("RC: %x\n", read_code);
			if (support_old)
				read_code = old_to_new(read_code);
		} while (read_code == last_read || (read_code & 0xff00) == 0x5c00);
	}
	else
	{
		do
		{
			read(fp, &read_code, 2);
			printf("RC: %x\n", read_code);
			if (support_old)
				read_code = old_to_new(read_code);
		} while ((read_code & 0xff00) == 0x5c00);

	}

	last_read = read_code;
	
	read_code %= 0x40;

	pthread_mutex_unlock( &mutex );

	return read_code;
}

int rc::old_to_new(int read_code)
{
	switch (read_code)
	{
	case RC2_1:
		return RC1_1;
	case RC2_2:
		return RC1_2;
	case RC2_3:
		return RC1_3;
	case RC2_4:
		return RC1_4;
	case RC2_5:
		return RC1_5;
	case RC2_6:
		return RC1_6;
	case RC2_7:
		return RC1_7;
	case RC2_8:
		return RC1_8;
	case RC2_9:
		return RC1_9;
	case RC2_0:
		return RC1_0;
	case RC2_STANDBY:
		return RC1_STANDBY;
	case RC2_HOME:
		return RC1_HOME;
	case RC2_DBOX:
		return RC1_DBOX;
	case RC2_RED:
		return RC1_RED;
	case RC2_GREEN:
		return RC1_GREEN;
	case RC2_YELLOW:
		return RC1_YELLOW;
	case RC2_BLUE:
		return RC1_BLUE;
	case RC2_OK:
		return RC1_OK;
	case RC2_VOLPLUS:
		return RC1_VOLPLUS;
	case RC2_VOLMINUS:
		return RC1_VOLMINUS;
	case RC2_MUTE:
		return RC1_MUTE;
	case RC2_HELP:
		return RC1_HELP;
	case RC2_UP:
		return RC1_UP;
	case RC2_DOWN:
		return RC1_DOWN;
	case RC2_RIGHT:
		return RC1_RIGHT;
	case RC2_LEFT:
		return RC1_LEFT;
	

	}
	return read_code;
}

int rc::get_number()
{
	int i, codes, number = -1;
	
	for (i = 0; i < NUMBER_RCS; i++)
	{
		for (codes = 0; codes < 10; codes++)
		{
			if (convert_code(last_read) == rc_codes[i][codes])
				number = codes;
		}
	}
	return number;
}


int rc::command_available()
{
	int rc;
	fd_set fds;
	struct timeval tv;
	
	pthread_mutex_lock( &mutex );

	FD_ZERO(&fds);
	FD_SET(fp,&fds);
	tv.tv_sec = tv.tv_usec = 0;
	
	rc = select(fp + 1, &fds, NULL, NULL, &tv);
	pthread_mutex_unlock( &mutex );
    if (rc < 0)
      return -1;

    return FD_ISSET(fp, &fds) ? 1 : 0;
}

int rc::get_command_without_removing()
{
	if (command_available())
	{
		unsigned short read_code;
		
		pthread_mutex_lock( &mutex );
		if (!repeat)
		{
			do
			{
				read(fp, &read_code, 2);
				printf("RC: %x\n", read_code);
				if (support_old)
					read_code = old_to_new(read_code);
			} while (read_code == last_read || (read_code & 0xff00) == 0x5c00);
		}
		else
		{
			do
			{
				read(fp, &read_code, 2);
				printf("RC: %x\n", read_code);
				if (support_old)
					read_code = old_to_new(read_code);
			} while ((read_code & 0xff00) == 0x5c00);
	
		}
	
		
		taken_commands.push(read_code);
		
		pthread_mutex_unlock( &mutex );
		return read_code %= 40;
	}
	else
		return -1;
}

