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

	Notifiers.insert( pair<uint, EventWatchdogNotifiers*>(WDE_VIDEOMODE, new EventWatchdogNotifiers));
	Notifiers.insert( pair<uint, EventWatchdogNotifiers*>(WDE_VCRONOFF, new EventWatchdogNotifiers));
	startThread();

}

void CEventWatchDog::startThread()
{
	pthread_mutex_init( &wd_mutex, NULL );

	if (pthread_create (&thrSender, NULL, watchdogThread, (void *) this) != 0 )
	{
		perror("CWatchdog: Create WatchDogThread failed\n");
	}
	bThreadRunning = true;
}

int CEventWatchDog::getVideoMode()
{
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

bool CEventWatchDog::getVCRMode()
{
	return false;
}

void CEventWatchDog::videoModeChanged( int nNewVideoMode )
{
	EventWatchdogNotifiers* notifiers = Notifiers.find(WDE_VIDEOMODE)->second;
	for (uint i=0; i<notifiers->size(); i++ )
	{
		((CAspectRatioNotifier*)(*notifiers)[i])->aspectRatioChanged( nNewVideoMode );
	}
	eventServer->sendEvent(CControldClient::EVT_MODECHANGED, 0, &nNewVideoMode, sizeof(nNewVideoMode));
}

void CEventWatchDog::vcrModeChanged( bool bBNewVCRMode )
{
	EventWatchdogNotifiers* notifiers = Notifiers.find(WDE_VCRONOFF)->second;
	for (uint i=0; i<notifiers->size(); i++ )
	{
		((CVideoModeNotifier*)(*notifiers)[i])->VCRModeChanged( bBNewVCRMode );
	}
	eventServer->sendEvent(CControldClient::EVT_VCRCHANGED, 0, &bBNewVCRMode, sizeof(bBNewVCRMode));
}

void* CEventWatchDog::watchdogThread (void *arg)
{
	CEventWatchDog* WatchDog = (CEventWatchDog*) arg;

	int fd_ev;
	fd_set rfds;

	struct event_t event;

	if ( (fd_ev = open( EVENT_DEVICE, O_RDWR ) ) < 0)
	{
		perror("open");
		return NULL;
	}

	if ( ioctl(fd_ev, EVENT_SET_FILTER, EVENT_VCR_OFF | EVENT_VCR_ON | EVENT_ARATIO_CHANGE /*| EVENT_VHSIZE_CHANGE*/ ) < 0 )
	{
		perror("ioctl");
		close(fd_ev);
		return NULL;
	}

	fcntl( fd_ev, F_SETFL, O_NONBLOCK );


	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd_ev, &rfds);

		int status = select(fd_ev+1, &rfds, NULL, NULL, NULL);

		if(FD_ISSET(fd_ev, &rfds))
		{
			status = read(fd_ev, &event, sizeof(event));
			if ( status == sizeof(event) )
			{
                if (event.event == EVENT_ARATIO_CHANGE)
                {
                	//printf("(event.event == EVENT_ARATIO_CHANGE)\n");
					int newVideoMode = WatchDog->getVideoMode();
					if ( (newVideoMode != WatchDog->lastVideoMode) && (newVideoMode != -1) )
					{
						pthread_mutex_trylock( &WatchDog->wd_mutex );
						WatchDog->lastVideoMode = (uint)newVideoMode;
						WatchDog->videoModeChanged( newVideoMode);
						pthread_mutex_unlock( &WatchDog->wd_mutex );
					}
				}
                else if ( (event.event == EVENT_VCR_ON) || (event.event == EVENT_VCR_OFF) )
                {
                	//printf("(event.event == EVENT_VCR)\n");
					int newVCRMode = (event.event == EVENT_VCR_ON);
					if ( (newVCRMode != WatchDog->lastVCRMode) )
					{
						pthread_mutex_trylock( &WatchDog->wd_mutex );
						WatchDog->lastVCRMode = newVCRMode;
						WatchDog->vcrModeChanged( newVCRMode );
						pthread_mutex_unlock( &WatchDog->wd_mutex );
					}
				}
			}
		}
	}
}

void CEventWatchDog::registerNotifier( uint watchdogEvent, CEventWatchdogNotifier* notifier )
{
	if (bThreadRunning)
		pthread_mutex_lock( &wd_mutex );

	Notifiers.find(watchdogEvent)->second->insert( Notifiers.find(watchdogEvent)->second->end(), notifier);

	if (watchdogEvent== WDE_VIDEOMODE)
	{
		videoModeChanged( getVideoMode() );
	}

	if (bThreadRunning)
		pthread_mutex_unlock( &wd_mutex );

	if (!bThreadRunning)
		startThread();
}

void CEventWatchDog::unregisterNotifier( uint watchdogEvent, CEventWatchdogNotifier* notifier )
{
	if (bThreadRunning)
		pthread_mutex_lock( &wd_mutex );

	EventWatchdogNotifiers* notifiers = Notifiers.find(watchdogEvent)->second;
	EventWatchdogNotifiers::iterator it;
	for (it=notifiers->end(); it>=notifiers->begin(); it--)
	{
		if (*it == notifier)
		{
			notifiers->erase(it);
		}
	}

	if (bThreadRunning)
		pthread_mutex_unlock( &wd_mutex );
}

