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

#include <dbox/event.h>

#include <eventserver.h>
#include <controldclient/controldclient.h>

#include <linux/dvb/video.h>
#include <sys/poll.h>

#include "eventwatchdog.h"

#define SAA7126_DEVICE	"/dev/dbox/saa0"
#define VIDEO_DEVICE	"/dev/dvb/adapter0/video0"

CEventWatchDog::CEventWatchDog()
{
	bThreadRunning = false;
	VideoMode = 0;
	Notifiers.insert( pair<uint, EventWatchdogNotifiers*>(WDE_VIDEOMODE, new EventWatchdogNotifiers));
	Notifiers.insert( pair<uint, EventWatchdogNotifiers*>(WDE_VCRONOFF, new EventWatchdogNotifiers));
	startThread();

}

void CEventWatchDog::startThread()
{
	pthread_mutex_init( &wd_mutex, NULL );

	if (pthread_create (&thrSender, NULL, CEventWatchDog::watchdogThread, (void *) this) != 0 )
	{
		perror("CWatchdog: Create WatchDogThread failed\n");
	}
	bThreadRunning = true;
}

int CEventWatchDog::getVideoMode()
{
	video_size_t size;
	int fd = open(VIDEO_DEVICE, O_RDONLY);

	if (fd == -1) {
		perror("[controld] " VIDEO_DEVICE);
		return 0;
	}

	if (ioctl(fd, VIDEO_GET_SIZE, &size) < 0) {
		perror("[controld] VIDEO_GET_SIZE");
		return 0;
	}

	return size.aspect_ratio;
}

int CEventWatchDog::getVCRMode()
{
	int val = 0;
	int fp = open("/dev/dbox/fp0",O_RDWR);

	if (fp >= 0) {
		ioctl(fp, FP_IOCTL_GET_VCR, &val);
		close(fp);
		//printf("getVCRMode= %d\n", val);
	}

	return val;
}

void CEventWatchDog::videoModeChanged( int nNewVideoMode )
{
	EventWatchdogNotifiers* notifiers = Notifiers.find(WDE_VIDEOMODE)->second;
	for (uint i=0; i<notifiers->size(); i++ )
	{
		((CAspectRatioNotifier*)(*notifiers)[i])->aspectRatioChanged( nNewVideoMode );
	}
	eventServer->sendEvent(CControldClient::EVT_MODECHANGED, CEventServer::INITID_CONTROLD, &nNewVideoMode, sizeof(nNewVideoMode));
}

void CEventWatchDog::vcrModeChanged( int nNewVCRMode )
{
	EventWatchdogNotifiers* notifiers = Notifiers.find(WDE_VCRONOFF)->second;
	for (uint i=0; i<notifiers->size(); i++ )
	{
		((CVCRModeNotifier*)(*notifiers)[i])->VCRModeChanged( nNewVCRMode );
	}
	eventServer->sendEvent(CControldClient::EVT_VCRCHANGED, CEventServer::INITID_CONTROLD, &nNewVCRMode, sizeof(nNewVCRMode));
}

void *CEventWatchDog::watchdogThread(void *arg)
{
	char *verb_aratio[] = { "4:3", "16:9", "2.21:1" };

	try {
		CEventWatchDog *WatchDog = (CEventWatchDog *)arg;
		int fd_ev, fd_video;

		if ((fd_ev = open(EVENT_DEVICE, O_RDWR | O_NONBLOCK)) < 0) {
			perror("[controld] " EVENT_DEVICE);
			pthread_exit(NULL);
		}

		if (ioctl(fd_ev, EVENT_SET_FILTER, EVENT_VCR_CHANGED) < 0) {
			perror("[controld] EVENT_SET_FILTER");
			close(fd_ev);
			pthread_exit(NULL);
		}

		if ((fd_video = open(VIDEO_DEVICE, O_RDONLY | O_NONBLOCK)) < 0) {
			perror("[controld] " VIDEO_DEVICE);
			close(fd_ev);
			pthread_exit(NULL);
		}

		struct pollfd pfd[2];
		pfd[0].fd = fd_ev;
		pfd[0].events = POLLIN;
		pfd[1].fd = fd_video;
		pfd[1].events = POLLPRI;

		while (poll(pfd, 2, -1) > 0) {
			if (pfd[0].revents & POLLIN) {
				struct event_t event;
				while (read(fd_ev, &event, sizeof(event)) == sizeof(event)) {
					if (event.event == EVENT_VCR_CHANGED) {
						int newVCRMode = WatchDog->getVCRMode();
						if ((newVCRMode != WatchDog->VCRMode)) {
							pthread_mutex_lock( &WatchDog->wd_mutex );

							WatchDog->VCRMode = newVCRMode;
							WatchDog->vcrModeChanged( newVCRMode );
							
							if(newVCRMode > 0)
							{
								//Set Aspect ratio of scart input signal (1->4:3 / 2->16:9)
								// vcr AR is saved in Bit 8-15, DVB AR is saved in bits 0-7
								WatchDog->VideoMode = (WatchDog->VideoMode & 0xFF) | ((newVCRMode-1) << 8);
								WatchDog->videoModeChanged(WatchDog->VideoMode);
							}
							pthread_mutex_unlock( &WatchDog->wd_mutex );
						}
					}
				}
			}

			if (pfd[1].revents & POLLPRI) {
				struct video_event event;
				
				if (ioctl(fd_video, VIDEO_GET_EVENT, &event) == -1) {
					perror("[controld] VIDEO_GET_EVENT");
				}
				else if (event.type == VIDEO_EVENT_SIZE_CHANGED) {
					printf("[controld] VIDEO_EVENT_SIZE_CHANGED %ux%u (%s -> %s)\n",
							event.u.size.w,
							event.u.size.h,
							verb_aratio[WatchDog->VideoMode&3],
							verb_aratio[event.u.size.aspect_ratio]);

					// DVB AR is saved in Bites 0-7
					if ((WatchDog->VideoMode & 0xFF) != event.u.size.aspect_ratio) {
						pthread_mutex_lock(&WatchDog->wd_mutex);
						WatchDog->VideoMode = (WatchDog->VideoMode & 0xFF00 ) | event.u.size.aspect_ratio;
						WatchDog->videoModeChanged(WatchDog->VideoMode);
						pthread_mutex_unlock(&WatchDog->wd_mutex);
					}
				}
			}
		}

		close(fd_video);
		close(fd_ev);
	}
	catch (std::exception& e)
	{
		fprintf(stderr, "[controld] caught std-exception ineventwatchdog %s!\n", e.what());
	}
	catch (...)
	{
	    fprintf(stderr, "[controld] caught exception in eventwatchdog!\n");
  	}

	pthread_exit(NULL);
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
	if (watchdogEvent== WDE_VCRONOFF)
	{
		vcrModeChanged( getVCRMode() );
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
