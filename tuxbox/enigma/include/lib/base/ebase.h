#ifndef __thread_h
#define __thread_h

#include <vector>
#include <list>
#include <map>
#include <libsig_comp.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>

class eApplication;

extern eApplication* eApp;

static inline bool operator<( const timeval &t1, const timeval &t2 )
{
	return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec);
}

static inline timeval &operator+=( timeval &t1, const timeval &t2 )
{
	t1.tv_sec += t2.tv_sec;
	if ( (t1.tv_usec += t2.tv_usec) >= 1000000 )
	{
		t1.tv_sec++;
		t1.tv_usec -= 1000000;
	}
	return t1;
}

static inline timeval operator+( const timeval &t1, const timeval &t2 )
{
	timeval tmp;
	tmp.tv_sec = t1.tv_sec + t2.tv_sec;
	if ( (tmp.tv_usec = t1.tv_usec + t2.tv_usec) >= 1000000 )
	{
		tmp.tv_sec++;
		tmp.tv_usec -= 1000000;
	}
	return tmp;
}

static inline timeval operator-( const timeval &t1, const timeval &t2 )
{
	timeval tmp;
	tmp.tv_sec = t1.tv_sec - t2.tv_sec;
	if ( (tmp.tv_usec = t1.tv_usec - t2.tv_usec) < 0 )
	{
		tmp.tv_sec--;
		tmp.tv_usec += 1000000;
	}
	return tmp;
}

static inline timeval operator-=( timeval &t1, const timeval &t2 )
{
	t1.tv_sec -= t2.tv_sec;
	if ( (t1.tv_usec -= t2.tv_usec) < 0 )
	{
		t1.tv_sec--;
		t1.tv_usec += 1000000;
	}
	return t1;
}

static inline timeval &operator+=( timeval &t1, const long msek )
{
	t1.tv_sec += msek / 1000;
	if ( (t1.tv_usec += (msek % 1000) * 1000) >= 1000000 )
	{
		t1.tv_sec++;
		t1.tv_usec -= 1000000;
	}
	return t1;
}

static inline timeval operator+( const timeval &t1, const long msek )
{
	timeval tmp;
	tmp.tv_sec = t1.tv_sec + msek / 1000;
	if ( (tmp.tv_usec = t1.tv_usec + (msek % 1000) * 1000) >= 1000000 )
	{
		tmp.tv_sec++;
		tmp.tv_usec -= 1000000;
	}
	return tmp;
}

static inline timeval operator-( const timeval &t1, const long msek )
{
	timeval tmp;
	tmp.tv_sec = t1.tv_sec - msek / 1000;
	if ( (tmp.tv_usec = t1.tv_usec - (msek % 1000)*1000) < 0 )
	{
		tmp.tv_sec--;
		tmp.tv_usec += 1000000;
	}
	return tmp;
}

static inline timeval operator-=( timeval &t1, const long msek )
{
	t1.tv_sec -= msek / 1000;
	if ( (t1.tv_usec -= (msek % 1000) * 1000) < 0 )
	{
		t1.tv_sec--;
		t1.tv_usec += 1000000;
	}
	return t1;
}

static inline timeval timeout_timeval ( const timeval & orig )
{
	timeval now;
  gettimeofday(&now,0);

	return orig-now;
}

static inline long timeout_usec ( const timeval & orig )
{
	timeval now;
  gettimeofday(&now,0);

	return (orig-now).tv_sec*1000000 + (orig-now).tv_usec;
}

class eThread
{
	pthread_t the_thread;
	int thread_id;
	static void *wrapper(void *ptr);
public:
	eThread();
	~eThread();

	virtual void thread()=0;
};

class eMainloop;

					// die beiden signalquellen: SocketNotifier...

class eSocketNotifier
{
public:
	enum { Read=POLLIN, Write=POLLOUT };
private:
	eMainloop &context;
	int fd;
	int state;
	int requested;		// requested events (POLLIN, ...)
public:
	Signal1<void, int> activated;

	eSocketNotifier(eMainloop *context, int fd, int req);
	~eSocketNotifier();
	
	void start();
	void stop();
	void activate(int what) { /*emit*/ activated(fd); }

	int getFD() { return fd; }
	int getRequested() { return requested; }
};

				// ... und Timer
class eTimer
{
	eMainloop &context;
	timeval nextActivation;
	long interval;
	bool bSingleShot;
	bool bActive;
public:
	eTimer(eMainloop *context=(eMainloop*)	eApp): bActive(false), context(*context) { }
	~eTimer()	{		if (bActive) stop();	}
	Signal0<void> timeout;
	void activate();
	bool isActive()	{		return bActive;	}
	void start(long msec, bool b=false);
	void stop();
	void changeInterval(long msek);
	timeval &getNextActivation() { return nextActivation; }
	struct less
	{
		bool operator() (const eTimer* t1, const eTimer* t2)
		{
			return (t1->nextActivation < t2->nextActivation);
		}
	};
};

			// werden in einer mainloop verarbeitet
class eMainloop : public Object
{
	std::map<int,eSocketNotifier*> notifiers;
	std::list<eTimer*> TimerList;
	void processOneEvent();
	bool app_exit_loop;
	bool app_quit_now;
	int loop_level;
public:
	eMainloop():loop_level(0)	{	}
	void addSocketNotifier(eSocketNotifier *sn);
	void removeSocketNotifier(eSocketNotifier *sn);

	void addTimer(eTimer* e)	
	{		
		TimerList.push_back(e);		
		TimerList.sort(eTimer::less());
	}
	void removeTimer(eTimer* e)	{		TimerList.remove(e);	}	
	int looplevel() { return loop_level; }
	
	void enter_loop();
	void exit_loop();

	int exec();		// recursive enter the loop
	
	void quit();	// leave all pending loops (recursive leave())
};


class eApplication: public eMainloop
{
public:
	eApplication()
	{
		if (!eApp)
			eApp = this;
	}
	~eApplication()
	{
		eApp = 0;
	}
};


class eMessagePump
{
	int fd[2];
public:
	int send(void *data, int len);
	int recv(void *data, int len); // blockierend
};

#endif
