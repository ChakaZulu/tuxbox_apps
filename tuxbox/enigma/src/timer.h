#ifndef __apps__enigma__timer_h
#define __apps__enigma__timer_h

class eService;

#include <core/base/estring.h>
#include <include/libsig_comp.h>
#include <core/dvb/dvb.h>
#include <core/dvb/record.h>

class eRecordingTimer: public Object
{
	void ready();
	void start();
	void pause();
	void stop();
	
	void serviceChanged(const eServiceReference &, int);
	void gotPMT(PMT*);
	
public:
	enum
	{
		stateDisabled, stateWaiting, stateReady, statePause, stateRunning, stateDone,
	};
	int state;
	
	enum
	{
		statusProgress, statusOK, statusFailure
	};
	int status;
	
	enum 
	{
		actionMessage, actionSwitch, actionRecord
	};
	
	int action;
	eServiceReference service;
	int eventid;
	time_t start_time;
	int duration; // in seconds
	eString description;
	
		// for actionMessage
	eString message;
		// for actionSwitch
	// nothing
		// for actionRecord
	eDVBRecorder *recorder;
	eString filename;
	
	void setState(int state);
	
	struct orderChron
	{
		bool operator()(const eRecordingTimer &a, const eRecordingTimer &b) const
		{
			return a.start_time < b.start_time;
		}
	};

	eRecordingTimer(time_t starttime);
	~eRecordingTimer();
};

class eTimerManager: public Object
{
	typedef std::multiset<eRecordingTimer, eRecordingTimer::orderChron> timerList;
	
	timerList recordtimer;
	
	void process();
	eTimer timer;

public:
	eTimerManager();
	~eTimerManager();
};

#endif
