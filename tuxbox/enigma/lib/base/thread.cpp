#include <lib/base/thread.h>
#include <stdio.h>
#include <lib/base/eerror.h>

void *eThread::wrapper(void *ptr)
{
	((eThread*)ptr)->thread();
	pthread_exit(0);
}

eThread::eThread()
{
	alive=0;
}

void eThread::run( int prio, int policy )
{
	alive=1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (prio)
	{
		struct sched_param p;
		p.__sched_priority=prio;
		pthread_attr_setschedpolicy(&attr, policy );
		pthread_attr_setschedparam(&attr, &p);
	}
	pthread_create(&the_thread, &attr, wrapper, this);
}                     

eThread::~eThread()
{
	if (alive)
		kill();
}

void eThread::kill()
{
	alive=0;
	eDebug("waiting for thread shutdown");
	pthread_join(the_thread, 0);
	eDebug("ok");
}
