#ifndef __apps__enigma__timer_h
#define __apps__enigma__timer_h

class eService;

#include <core/base/estring.h>
#include <include/libsig_comp.h>
#include <core/dvb/dvb.h>
#include <core/dvb/record.h>

class eRecordingTimer: public Object
{
	void rec_ready();
	void rec_start();
	void rec_pause();
	void rec_stop();
	
	void serviceChanged(const eServiceReference &, int);
	void gotPMT(PMT*);
	eTimer timer;
	void timeout();
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
	
	int disable();
	int enable();
	
	int ready();
	int start();
	
	int pause();
	int resume();
	
	int abort();
	
	eRecordingTimer();
	~eRecordingTimer();
};

class eTimerManager: public Object
{
	typedef ePtrList<eRecordingTimer> timerList;
	
	timerList recordtimer;
	
	void process();
public:
	eTimerManager();
	~eTimerManager();
};

#endif
