#include "thread.h"
#include <stdio.h>

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
	printf("waiting for thread shutdown\n");
	pthread_join(the_thread, 0);
	printf("ok\n");
}

