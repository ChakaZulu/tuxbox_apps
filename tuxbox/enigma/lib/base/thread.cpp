#include "thread.h"
#include <stdio.h>
#include <core/base/eerror.h>

void *eThread::wrapper(void *ptr)
{
	((eThread*)ptr)->thread();
	pthread_exit(0);
}

eThread::eThread()
{
}

void eThread::run()
{
	pthread_create(&the_thread, 0, wrapper, this);
}

eThread::~eThread()
{
	eDebug("waiting for thread shutdown");
	pthread_join(the_thread, 0);
	eDebug("ok");
}

