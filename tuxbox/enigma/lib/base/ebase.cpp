#include "ebase.h"

#include <fcntl.h>
#include <unistd.h>

eSocketNotifier::eSocketNotifier(eMainloop *context, int fd, int requested, bool startnow): fd(fd), context(*context), requested(requested)
{
	state=0;
	
	if (startnow)	
		start();
}

eSocketNotifier::~eSocketNotifier()
{
	stop();
}

void eSocketNotifier::start()
{
	if (state)
		stop();

	context.addSocketNotifier(this);
	state=1;
}

void eSocketNotifier::stop()
{
	if (state)
		context.removeSocketNotifier(this);

	state=0;
}

					// timer
void eTimer::start(long msek, bool singleShot)
{
	if (bActive)
		stop();

	bActive = true;
	bSingleShot = singleShot;
	interval = msek;
 	gettimeofday(&nextActivation, 0);		
	nextActivation += msek;
	context.addTimer(this);
}

void eTimer::stop()
{
	if (bActive)
	{
		bActive=false;
		context.removeTimer(this);
	}
}

void eTimer::changeInterval(long msek)
{
	if (bActive)  // Timer is running?
	{
		context.removeTimer(this);	 // then stop
		nextActivation -= interval;  // sub old interval
	}
	else
		bActive=true;	// then activate Timer

	interval = msek;   			 			// set new Interval
	nextActivation += interval;		// calc nextActivation

	context.addTimer(this);				// add Timer to context TimerList
}

void eTimer::activate()   // Internal Funktion... called from eApplication
{
//	printf("Timer emitted\n");
	context.removeTimer(this);

	if (!bSingleShot)
	{
		nextActivation += interval;
		context.addTimer(this);
	}
	else
		bActive=false;

	/*emit*/ timeout();
}

// mainloop

void eMainloop::addSocketNotifier(eSocketNotifier *sn)
{
	notifiers.insert(std::pair<int,eSocketNotifier*> (sn->getFD(), sn));
}

void eMainloop::removeSocketNotifier(eSocketNotifier *sn)
{
	notifiers.erase(sn->getFD());
}

void eMainloop::processOneEvent()
{
	int cnt=0;
	int fdAnz = notifiers.size();
	pollfd* pfd = new pollfd[fdAnz];  // make new pollfd array

// fill pfd array
	std::map<int,eSocketNotifier*>::iterator it(notifiers.begin());
	for (int i=0; i < fdAnz; i++, it++)
	{
		pfd[i].fd = it->first;
		pfd[i].events = it->second->getRequested();
	}

// process pending timers...
	long usec;

	while (TimerList && (usec = timeout_usec( TimerList.begin()->getNextActivation() ) ) <= 0 )
		TimerList.begin()->activate();

	int ret=poll(pfd, fdAnz, TimerList ? usec / 1000 : -1);  // milli .. not micro seks

	if (ret>0)
	{
//		printf("bin aussem poll raus und da war was\n");
		for (int i=0; i < fdAnz ; i++)
		{
			if( notifiers.find(pfd[i].fd) == notifiers.end())
				continue;

			int req = notifiers[pfd[i].fd]->getRequested();

			if ( (pfd[i].revents & req) == req)
			{
				notifiers[pfd[i].fd]->activate(pfd[i].revents);

				if (!--ret)
					break;
			}
		}
	}
	else if (ret<0)
		printf("poll made error\n");

		// check Timers...
	while ( TimerList && timeout_usec( TimerList.begin()->getNextActivation() ) <= 0 )
		TimerList.begin()->activate();

	delete [] pfd;
}


int eMainloop::exec()
{
	if (!loop_level)
	{
		app_quit_now = false;
		enter_loop();
	}
}

void eMainloop::enter_loop()
{
	loop_level++;

	// Status der vorhandenen Loop merken
	bool old_exit_loop = app_exit_loop;
	
	app_exit_loop = false;

	while (!app_exit_loop && !app_quit_now)
	{
		processOneEvent();
	}	

	// wiederherstellen der vorherigen app_exit_loop
	app_exit_loop = old_exit_loop;

	loop_level--;

	if (!loop_level)
	{
			// do something here on exit the last loop
	}
}

void eMainloop::exit_loop()  // call this to leave the current loop
{
	app_exit_loop = true;	
}

void eMainloop::quit()   // call this to leave all loops
{
	app_quit_now = true;
}

eApplication* eApp = 0;
