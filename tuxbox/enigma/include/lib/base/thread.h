#ifndef __core_base_thread_h
#define __core_base_thread_h

#include <pthread.h>

class eThread
{
	pthread_t the_thread;
	static void *wrapper(void *ptr);
	int alive;
public:
	eThread();
	virtual ~eThread();
	
	void run();

	virtual void thread()=0;
	
	void kill();
};

#endif
