#ifndef __ebase_h
#define __ebase_h

#include <vector>
#include <map>
#include <sys/poll.h>
#include <sys/time.h>
#include <asm/types.h>
#include <time.h>
#include <pthread.h>

#include <lib/base/eptrlist.h>
#include <libsig_comp.h>

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
	if ( (orig-now).tv_sec > 2000 )
		return 2000*1000*1000;
	return (orig-now).tv_sec*1000000 + (orig-now).tv_usec;
}

class eMainloop;

					// die beiden signalquellen: SocketNotifier...

/**
 * \brief Gives a callback when data on a file descriptor is ready.
 *
 * This class emits the signal \c eSocketNotifier::activate whenever the
 * event specified by \c req is available.
 */
class eSocketNotifier
{
	friend class eMainloop;
public:
	enum { Read=POLLIN, Write=POLLOUT, Priority=POLLPRI, Error=POLLERR, Hungup=POLLHUP };
private:
	eMainloop &context;
	int fd;
	int state;
	int requested;		// requested events (POLLIN, ...)
	void activate(int what) { /*emit*/ activated(what); }
public:
	/**
	 * \brief Constructs a eSocketNotifier.
	 * \param context The thread where to bind the socketnotifier to. The signal is emitted from that thread.
	 * \param fd The filedescriptor to monitor. Can be a device or a socket.
	 * \param req The events to watch to, normally either \c Read or \c Write. You can specify any events that \c poll supports.
	 * \param startnow Specifies if the socketnotifier should start immediately.
	 */
	eSocketNotifier(eMainloop *context, int fd, int req, bool startnow=true);
	~eSocketNotifier();

	Signal1<void, int> activated;

	void start();
	void stop();
	bool isRunning() { return state; }

	int getFD() { return fd; }
	int getRequested() { return requested; }
	void setRequested(int req) { requested=req; }
};

				// ... und Timer
/**
 * \brief Gives a callback after a specified timeout.
 *
 * This class emits the signal \c eTimer::timeout after the specified timeout.
 */
class eTimer
{
	friend class eMainloop;
	eMainloop &context;
	timeval nextActivation;
	long interval;
	bool bSingleShot;
	bool bActive;
	inline void recalc(int);
public:
	/**
	 * \brief Constructs a timer.
	 *
	 * The timer is not yet active, it has to be started with \c start.
	 * \param context The thread from which the signal should be emitted.
	 */
	eTimer(eMainloop *context): context(*context), bActive(false) { }
	~eTimer() { if (bActive) stop(); }

	Signal0<void> timeout;
	void activate();

	bool isActive() { return bActive; }
	timeval &getNextActivation() { return nextActivation; }

	void start(long msec, bool b=false);
	void stop();
	void changeInterval(long msek);
	bool operator<(const eTimer& t) const { return nextActivation < t.nextActivation; }
	void startLongTimer( int seconds );
};

			// werden in einer mainloop verarbeitet
class eMainloop
{
	friend class eTimer;
	friend class eSocketNotifier;
	std::map<int, eSocketNotifier*> notifiers;
	ePtrList<eTimer> TimerList;
	bool app_exit_loop;
	bool app_quit_now;
	int loop_level;
	void processOneEvent();
	int retval;
	int timer_offset;
	pthread_mutex_t recalcLock;
	inline void doRecalcTimers();
	inline void addSocketNotifier(eSocketNotifier *sn);
	inline void removeSocketNotifier(eSocketNotifier *sn);
	inline void addTimer(eTimer* e)	{		TimerList.insert_in_order(e);	}
	inline void removeTimer(eTimer* e)	{		TimerList.remove(e);	}
public:
	static ePtrList<eMainloop> existing_loops;
	eMainloop()
		:app_quit_now(0),loop_level(0),retval(0),timer_offset(0)
	{
		existing_loops.push_back(this);
		pthread_mutex_init(&recalcLock, 0);
	}
	~eMainloop()
	{
		existing_loops.remove(this);
		pthread_mutex_destroy(&recalcLock);
	}
	int looplevel() { return loop_level; }

	int exec();  // recursive enter the loop
	void quit(int ret=0); // leave all pending loops (recursive leave())
	void enter_loop();
	void exit_loop();
	void setTimerOffset( int );
	int getTimerOffset() { return timer_offset; }
	bool isAppQuitNowSet() { return app_quit_now; }
};

/**
 * \brief The application class.
 *
 * An application provides a mainloop, and runs in the primary thread.
 * You can have other threads, too, but this is the primary one.
 */
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
#endif
