/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

#ifndef __neutrino_timermanager__
#define __neutrino_timermanager__

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <map>
#include <string>


class CTimerEvent;
class CTimerManager
{
	//singleton
	private:
		CTimerManager();
		int				eventID;
		std::map<int, CTimerEvent*>	events;
		CTimerEvent			*nextEvent();
		pthread_t			thrTimer;

		static void* timerThread(void *arg);

	public:
		static CTimerManager* getInstance();
		
		int addEvent(CTimerEvent*);
		void removeEvent(int eventID);
		CTimerEvent* getNextEvent();
};


class CTimerEvent
{
	public:
		void*			eventData;
		struct tm		alarmtime;

		virtual void fireEvent(){};
};

class CTimerEvent_Shutdown : public CTimerEvent
{
	public:
		virtual void fireEvent();
};


#endif
