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
$Log: checker.cpp,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/frontend.h>
#include <ost/audio.h>
#include <ost/sec.h>
#include <ost/sec.h>
#include <ost/ca.h>
#include <dbox/avs_core.h>

#include "checker.h"
#include "pthread.h"

#define BSIZE 10000

static int mode_16_9; // hell, this IS damn ugly

void checker::set_16_9_mode(int mode)
{
	mode_16_9 = mode;
	printf("set 16:9: %d\n", mode_16_9);
}

int checker::start_16_9_thread()
{
	
  int status;
  
  status = pthread_create( &timeThread,
                           NULL,
                           start_16_9_checker,
                           (void *)this );
  return status;

}

void fnc(int i, int mode_16_9)
{
	int	avs = open("/dev/dbox/avs0",O_RDWR);
	int vid = open("/dev/ost/video0", O_RDWR);
	ioctl(avs, AVSIOSFNC, &i);
	if (i == 1)
		ioctl(vid, VIDEO_SET_DISPLAY_FORMAT, VIDEO_CENTER_CUT_OUT);
	if (i == 0)
		if (mode_16_9 == 2)
			ioctl(vid, VIDEO_SET_DISPLAY_FORMAT, VIDEO_LETTER_BOX);
		else
			ioctl(vid, VIDEO_SET_DISPLAY_FORMAT, VIDEO_PAN_SCAN);
	close(avs);
	close(vid);
}


int checker::get_16_9_mode()
{
	printf("16_9: %d\n", mode_16_9);
	return mode_16_9;
}

void* checker::start_16_9_checker( void * this_ptr )
{
	int check = 0;
	int laststat_mode = 0;
	FILE *fp;
	char buffer[100];
	int laststat = 0;
	
	fnc(0, mode_16_9);

	while(1)
	{
		fp = fopen("/proc/bus/bitstream", "r");
		while (!feof(fp))
		{
			fgets(buffer, 100, fp);
			sscanf(buffer, "A_RATIO: %d", &check);
		
		}	
		fclose(fp);
		
		if (check == 3) // 16:9
		{
			if (laststat != 1 || laststat_mode != mode_16_9)
			{
				if (mode_16_9 == 0)
					fnc(1, mode_16_9);
				else
					fnc(0, mode_16_9);
				laststat_mode = mode_16_9;
				laststat = 1;
			}
			
		}
		if (check != 3)
		{
			if (laststat != 0)
			{
				fnc(0, mode_16_9);
				laststat = 0;
			}
		}
		sleep(1); // alle 30 Minuten die Zeit neu setzen...
	}

}
