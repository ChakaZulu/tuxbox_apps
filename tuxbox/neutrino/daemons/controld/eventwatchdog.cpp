/*
	Neutrino-GUI  -   DBoxII-Project

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

#include "eventwatchdog.h"
#include <dbox/event.h>

CEventWatchDog::CEventWatchDog()
{
	bThreadRunning = false;
	bCheckVideoMode = false;
	bCheckParentalLock = false;

	Notifiers.insert( pair<uint, EventWatchdogNotifiers*>(WDE_VIDEOMODE, new EventWatchdogNotifiers));
//	Notifiers.insert( pair<uint, EventWatchdogNotifiers*>(WDE_PARENTALLOCK, new EventWatchdogNotifiers));
	startThread();

}

void CEventWatchDog::startThread()
{
	pthread_mutex_init( &wd_mutex, NULL );

	if (pthread_create (&thrSender, NULL, watchdogThread, (void *) this) != 0 )
	{
		perror("CWatchdog: Create WatchDogThread failed\n");
	}
//	printf("Thread running\n");
	bThreadRunning = true;
}

/*
void CEventWatchDog::stopThread()
{
}
*/
int CEventWatchDog::getVideoMode( bool bWaitForEvent = true)
{
	int fd;
	int err;
	unsigned long arg;

	struct event_t event;

	if((fd = open(EVENT_DEVICE,O_RDWR)) < 0)
	{
		perror("open");
		return -1;
	}

	arg = /*EVENT_VCR_OFF | EVENT_VHSIZE_CHANGE | */ EVENT_ARATIO_CHANGE;

	if ( ioctl(fd,EVENT_SET_FILTER,arg) < 0 )
	{
		perror("ioctl");
	}
	else if (bWaitForEvent)
	{
		if ( read(fd,&event,sizeof(event_t)) <= 0 )
			perror("read");
	}
//	else
//		printf("event: %d\n",event.event);

	close(fd);

	if ((event.event == EVENT_ARATIO_CHANGE) || (!bWaitForEvent))
	{
//		printf("reading bitstream\n");
		FILE* fdEvent = fopen("/proc/bus/bitstream", "rt");
		if (fdEvent==NULL)
		{
			printf("error while opening proc-bitstream\n" );
			return -1;
		}

		int bitInfo[10];
		char *key,*tmpptr,buf[100];
		int value, pos=0;
		fgets(buf,29,fdEvent);//dummy
		while(!feof(fdEvent))
		{
			if(fgets(buf,29,fdEvent)!=NULL)
			{
				buf[strlen(buf)-1]=0;
				tmpptr=buf;
				key=strsep(&tmpptr,":");
				for(;tmpptr[0]==' ';tmpptr++);
				value=atoi(tmpptr);
				//printf("%s: %d\n",key,value);
				bitInfo[pos]= value;
				pos++;
			}
		}
		fclose(fdEvent);

		return(bitInfo[2]);
	}
	else
	{
//		printf("returning lastVideoMode\n");
		return(lastVideoMode);
	}
}

int CEventWatchDog::getParentalLock()
{
	return -1;
}

void CEventWatchDog::videoModeChanged( int nNewVideoMode)
{
//	printf("...videomodeChanegd \n");

	EventWatchdogNotifiers* notifiers = Notifiers.find(WDE_VIDEOMODE)->second;
//	printf("...notifiers found \n");
//	printf("...Anzahl: %d \n", notifiers->size());

	for (uint i=0; i<notifiers->size(); i++ )
	{
//		printf("...before notify \n");
		((CAspectRatioNotifier*)(*notifiers)[i])->aspectRatioChanged( nNewVideoMode);
//		printf("...notify done\n");
	}
}

void* CEventWatchDog::watchdogThread (void *arg)
{
	CEventWatchDog* WatchDog = (CEventWatchDog*) arg;

	while (1)
	{

		if (WatchDog->bCheckVideoMode)
		{
			int newVideoMode = WatchDog->getVideoMode();

			if ((newVideoMode != WatchDog->lastVideoMode) && (newVideoMode != -1))
			{
				pthread_mutex_trylock( &WatchDog->wd_mutex );
				WatchDog->lastVideoMode = (uint)newVideoMode;
				WatchDog->videoModeChanged( newVideoMode);
				pthread_mutex_unlock( &WatchDog->wd_mutex );
			}
		}
		usleep(1000000);
	}
}

void CEventWatchDog::registerNotifier( uint watchdogEvent, CEventWatchdogNotifier* notifier )
{
//	printf("Watchdog: registerNotifier\n");

	if (bThreadRunning)
		pthread_mutex_lock( &wd_mutex );

//	printf("Registering ...");
	Notifiers.find(watchdogEvent)->second->insert( Notifiers.find(watchdogEvent)->second->end(), notifier);
//	printf("...done \n");

	bCheckVideoMode    = Notifiers.find(WDE_VIDEOMODE)->second->size() > 0;

	if (bCheckVideoMode)
	{
		videoModeChanged( getVideoMode( false));
	}

//	bCheckParentalLock = Notifiers.find(WDE_PARENTALLOCK)->second->size() > 0;

	if (bThreadRunning)
		pthread_mutex_unlock( &wd_mutex );

	if (!bThreadRunning)
		startThread();
}

void CEventWatchDog::unregisterNotifier( uint watchdogEvent, CEventWatchdogNotifier* notifier )
{
//	printf("Watchdog: unregisterNotifier\n");
	if (bThreadRunning)
		pthread_mutex_lock( &wd_mutex );

//	printf("Watchdog: got mutex\n");
	EventWatchdogNotifiers* notifiers = Notifiers.find(watchdogEvent)->second;
	EventWatchdogNotifiers::iterator it;
	for (it=notifiers->end(); it>=notifiers->begin(); it--)
	{
		if (*it == notifier)
		{
//			printf("Found registered, removing ...");
			notifiers->erase(it);
//			printf("...done \n");
		}
	}

	bCheckVideoMode    = Notifiers.find(WDE_VIDEOMODE)->second->size() > 0;
//	bCheckParentalLock = callbackFunctions.find(WDE_PARENTALLOCK)->second->size() > 0;

	if (bThreadRunning)
		pthread_mutex_unlock( &wd_mutex );
}

