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

#ifndef __eventwatchdog__
#define __eventwatchdog__

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_TRIPLEDRAGON
#include <avs/avs_inf.h>
#include <tddevices.h>
#define AVS_DEVICE "/dev/" DEVICE_NAME_AVS
#endif

#if defined HAVE_DBOX_HARDWARE || defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
#include <dbox/fp.h>
#define EVENT_DEVICE "/dev/dbox/event0"
#endif

#include <string>
#include <vector>
#include <map>


using namespace std;

extern  CEventServer        *eventServer;

// Events which can occur
#define WDE_VIDEOMODE 		(uint)1		// Videomode changed from 4:3 to 16:9 or from 16:9 to 4:3
#define WDE_VCRONOFF 		(uint)2		// VCR turned on or off

class CEventWatchdogNotifier
{
};

typedef vector<CEventWatchdogNotifier*> EventWatchdogNotifiers;

class CAspectRatioNotifier : public CEventWatchdogNotifier
{
	public:
		virtual void aspectRatioChanged( int newAspectRatio ) = 0;
};

class CVCRModeNotifier : public CEventWatchdogNotifier
{
	public:
		virtual void VCRModeChanged( int newVCRMode ) = 0;
};

class CEventWatchDog
{
  private :
	bool				bThreadRunning;
	pthread_mutex_t 	wd_mutex;
	pthread_t       	thrSender;

	map<uint, EventWatchdogNotifiers*>	Notifiers;

	int getVideoMode();
	int getVCRMode();

	void startThread();
	void videoModeChanged( int nNewVideoMode );
	void vcrModeChanged( int nNewVCRMode );

  public :
	int 	VideoMode;
	int    	VCRMode;
	CEventWatchDog();
	~CEventWatchDog();

	static void * watchdogThread (void *arg);
	void registerNotifier( uint watchdogEvent, CEventWatchdogNotifier* notifier );
	void unregisterNotifier( uint watchdogEvent, CEventWatchdogNotifier* notifier );
};

#endif
