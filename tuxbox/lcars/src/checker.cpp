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
Revision 1.8  2002/09/18 10:48:37  obi
use devfs devices

Revision 1.7  2002/06/15 02:33:03  TheDOC
some changes + bruteforce-channelscan for cable

Revision 1.6  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.5  2002/05/18 04:31:02  TheDOC
Warningelimination

Revision 1.4  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.3  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.4  2001/12/19 03:23:01  tux
event mit 16:9-Umschaltung

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

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
#include <dbox/event.h>
#include <iostream>

#include "checker.h"
#include "pthread.h"

#define BSIZE 10000

static int mode_16_9; // hell, this IS damn ugly

checker::checker(settings *s, hardware *h)
{
	setting = s;
	hardware_obj = h;
	laststat = 0;
	laststat_mode = 0;
}

void checker::set_16_9_mode(int mode)
{
	mode_16_9 = mode;
	//printf("set 16:9: %d\n", mode_16_9);
}

int checker::startEventThread()
{
	int status;

	status = pthread_create( &eventThread,
	                         NULL,
	                         startEventChecker,
	                         (void *)this );
	return status;
}

void checker::fnc(int i, int mode_16_9)
{
	int	avs = open("/dev/dbox/avs0",O_RDWR);
	int vid = open("/dev/dvb/card0/video0", O_RDWR);
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
	//printf("16_9: %d\n", mode_16_9);
	return mode_16_9;
}

void* checker::startEventChecker(void* object)
{
	checker *c = (checker *) object;
	struct event_t event;
	int fd;


	if((fd = open("/dev/dbox/event0", O_RDWR)) < 0)
	{
		perror("open");
		return false;
	}
	int old_vcr_mode = c->get_16_9_mode();

	while(1)
	{
		read(fd, &event, sizeof(event_t));
		//std::cout << "Event: " << event.event << std::endl;
		if (event.event == EVENT_VCR_CHANGED)
		{
			//std::cout << "Event: EVENT_VCR_CHANGED" << std::endl;
			switch (c->hardware_obj->getVCRStatus())
			{
			case VCR_STATUS_ON:
				//std::cout << "Event: EVENT_VCR_ON" << std::endl;
				//std::cout << "Status: " << (c->hardware_obj->getAvsStatus() & 0x0c) << std::endl;
				if (c->hardware_obj->vcrIsOn())
				{
					//std::cout << "ON" << std::endl;
					c->hardware_obj->fnc(2);

				}
				else
					//std::cout << "Off" << std::endl;
					if (c->setting->getSwitchVCR())
						if (!c->hardware_obj->vcrIsOn())
						{
							c->hardware_obj->switch_vcr();
							old_vcr_mode = c->get_16_9_mode();
							c->set_16_9_mode(1);
						}
				break;
			case VCR_STATUS_OFF:
				//std::cout << "Event: EVENT_VCR_OFF" << std::endl;
				//std::cout << "Status: " << (c->hardware_obj->getAvsStatus() & 0x0c) << std::endl;
				if (!c->hardware_obj->vcrIsOn())
				{
					//std::cout << "ON" << std::endl;
					c->hardware_obj->fnc(0);
				}
				else
					//std::cout << "Off" << std::endl;
					if (c->setting->getSwitchVCR())
						if (c->hardware_obj->vcrIsOn())
						{
							c->hardware_obj->switch_vcr();
							c->set_16_9_mode(old_vcr_mode);
						}
				break;
			case VCR_STATUS_16_9:
				//std::cout << "Event: EVENT_VCR_16:9" << std::endl;
				//std::cout << "Status: " << (c->hardware_obj->getAvsStatus() & 0x0c) << std::endl;
				if (c->hardware_obj->vcrIsOn())
				{
					//std::cout << "ON" << std::endl;
					c->hardware_obj->fnc(1);

				}
				else
					//std::cout << "Off" << std::endl;
					//std::cout << "16:9 switch on vcr" << std::endl;

					break;
			}
		}
		else if (event.event == EVENT_ARATIO_CHANGE)
		{
			//std::cout << "ARATIO-Change Event: " << c->laststat << " - " << c->laststat_mode << std::endl;

			c->aratioCheck();

		}
		//else
		//std::cout << "UNKNOWN EVENT!!!! PLEASE REPORT!!! " << event.event << std::endl;

		/*		else if (event.event == EVENT_SBVCR_CHANGE)
				{
					//std::cout << "Event: EVENT_SBVCR_CHANGE" << std::endl;
					//std::cout << "Status: " << (c->hardware_obj->getAvsStatus() & 0x0c) << std::endl;
					if (c->hardware_obj->vcrIsOn())
					{
						// TODO add code when driver supports it
						//std::cout << "ON" << std::endl;

					}
					else
						//std::cout << "Off" << std::endl;
					//std::cout << "16:9 switch on vcr" << std::endl;
				}*/

		/*else if (event.event == EVENT_VCR_OFF)
		{
		//std::cout << "Event: EVENT_VCR_OFF" << std::endl;
		//std::cout << "Status: " << (c->hardware_obj->getAvsStatus() & 0x0c) << std::endl;
		if (c->setting->getSwitchVCR())
			if (c->hardware_obj->vcrIsOn())
			{
				c->hardware_obj->switch_vcr();
				c->set_16_9_mode(old_vcr_mode);
			}
		}*/
	}
	close(fd);
}

void checker::aratioCheck()
{
	int check = hardware_obj->getARatio();

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
}
