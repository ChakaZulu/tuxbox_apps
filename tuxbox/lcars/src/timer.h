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

#ifndef	TIMER_H
#define TIMER_H

#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <map>
#include <string>
#include <unistd.h>

#include "hardware.h"
#include "channels.h"
#include "zap.h"
#include "tuner.h"
#include "osd.h"

struct timer_entry
{
	time_t starttime;
	int duration;
	int status; // 0 - active, 1 - done, 2 - disabled, 3 - failed, 4 - running
	int type; // 0 - shutdown, 1 - reboot, 2 - zap, 3 - record
	int channel;
	char comment[100];
};

class timer
{
	pthread_t timerThread;
	pthread_mutex_t mutex;
 
    static void* start_timer( void * );

	std::multimap<time_t, struct timer_entry> timer_list;
    
	hardware *hardware_obj;
	channels *channels_obj;
	zap *zap_obj;
	tuner *tuner_obj;
	osd *osd_obj;

	time_t dumped_starttimes[20];
	int dumped_channels[20];
public:	
	timer(hardware *h, channels *c, zap *z, tuner *t, osd *o);
	int start_thread();

	void runTimer();
	bool isEmpty();

	void addTimer(time_t starttime, int type, std::string comment, int duration = 0, int channel = -1);
	void dumpTimer();
	int getDumpedChannel(int i) { return dumped_channels[i]; }
	time_t getDumpedStarttime(int i) { return dumped_starttimes[i]; }
	void rmTimer(int channel, time_t starttime);
	void saveTimer();
	void loadTimer();
};

#endif
