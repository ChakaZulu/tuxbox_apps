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

#include "timer.h"

timer::timer(hardware *h, channels *c, zap *z, tuner *t, osd *o)
{
	hardware_obj = h;
	channels_obj = c;
	zap_obj = z;
	tuner_obj = t;
	osd_obj = o;
}

int timer::start_thread()
{
	
	int status;

	printf("1\n");
	pthread_mutex_init(&mutex, NULL);
	printf("1\n");
	status = pthread_create( &timerThread,
                           NULL,
                           start_timer,
                           (void *)this );
	return status;

}

void* timer::start_timer( void * this_ptr )
{
	timer *t = (timer *) this_ptr;

	while(1)
	{
		while(t->isEmpty())
		{
			sleep(5);
		}
		
		t->runTimer();
	}
}

void timer::addTimer(time_t starttime, int type, std::string comment, int duration = 0, int channel = -1)
{
	struct timer_entry new_timer;

	memset (&new_timer, 0, sizeof(struct timer_entry));

	new_timer.starttime = starttime;
	new_timer.channel = channel;
	new_timer.duration = duration;
	new_timer.status = 0;
	new_timer.type = type;
	strcpy(new_timer.comment, comment.c_str());
	
	printf("New timer: %s %d\n", ctime(&starttime), new_timer.type);

	pthread_mutex_lock( &mutex );
	timer_list.insert(std::pair<time_t, struct timer_entry>(starttime, new_timer));
	pthread_mutex_unlock( &mutex );

}

bool timer::isEmpty()
{
	pthread_mutex_lock( &mutex );
	bool empty = (timer_list.size() == 0);
	pthread_mutex_unlock( &mutex );

	return empty;
}

void timer::runTimer()
{
	pthread_mutex_lock( &mutex );
	std::multimap<time_t, struct timer_entry>::iterator it = timer_list.begin();
	struct timer_entry act_timer = (*it).second;

	if (act_timer.starttime > time(0))
	{
		pthread_mutex_unlock( &mutex );

		return;
	}

	timer_list.erase(it);
	pthread_mutex_unlock( &mutex );
	
	saveTimer();

	if (act_timer.type == 0)
		(*hardware_obj).shutdown();
	else if (act_timer.type == 1)
		(*hardware_obj).reboot();
	else if (act_timer.type == 2)
	{
		int last_channel = (*channels_obj).getCurrentChannelNumber();
		(*channels_obj).setCurrentChannel(act_timer.channel);
		(*channels_obj).zapCurrentChannel(zap_obj, tuner_obj);
		if (act_timer.duration != 0)
			addTimer(time(0) + act_timer.duration, 2, 0, last_channel);
		(*channels_obj).setCurrentOSDProgramInfo(osd_obj);
					
		(*channels_obj).receiveCurrentEIT();
		(*channels_obj).setCurrentOSDProgramEIT(osd_obj);
		(*channels_obj).updateCurrentOSDProgramAPIDDescr(osd_obj);

	}
}

void timer::dumpTimer()
{
	int position = 1;
	pthread_mutex_lock( &mutex );
	for (std::multimap<time_t, struct timer_entry>::iterator it = timer_list.begin(); it != timer_list.end(); ++it, position++)
	{
		time_t starttime = (*it).second.starttime;
		struct tm *t;
		t = localtime(&starttime);
		char timetxt[20];
		strftime(timetxt, sizeof timetxt, "%a, %H:%M", t);
		char text[300];
		sprintf(text, "%s Ch: %d - ", timetxt, (*it).second.channel);

		char comment[300];
		strcpy(comment, (*it).second.comment);
		if (strlen(comment) != 0)
			strcat(text, comment);
		else
			strcat(text, "Timer");
		osd_obj->addMenuEntry(position, text);

		dumped_starttimes[position] = (*it).second.starttime;
		dumped_channels[position] = (*it).second.channel;


	}
	pthread_mutex_unlock( &mutex );
}

void timer::rmTimer(int channel, time_t starttime)
{
	printf("Start Removing Timer\n");
	pthread_mutex_lock( &mutex );
	timer_list.erase(timer_list.find(starttime));
	pthread_mutex_unlock( &mutex );
	printf("End Removing Timer\n");
}

void timer::saveTimer()
{
	int fd;
	fd = open("/var/lcars/timer.dat", O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		perror("Couldn't save timer\n");
		return;
	}

	pthread_mutex_lock(&mutex);
	
	for (std::multimap<time_t, struct timer_entry>::iterator it = timer_list.begin(); it != timer_list.end(); ++it)
	{
		struct timer_entry tmp_timer = (*it).second;
		write(fd, &tmp_timer, sizeof(timer_entry));
	}

	pthread_mutex_unlock(&mutex);
	
	close(fd);
}

void timer::loadTimer()
{
	int fd = open("/var/lcars/timer.dat", O_RDONLY);
	if (fd < 0)
	{
		perror("Couldn't load timer\n");
		return;
	}

	pthread_mutex_lock(&mutex);
	
	struct timer_entry tmp_timer;
	while(read(fd, &tmp_timer, sizeof(timer_entry)))
	{
		timer_list.insert(std::pair<time_t, struct timer_entry>(tmp_timer.starttime, tmp_timer));
	}

	pthread_mutex_unlock(&mutex);

	close(fd);
}

