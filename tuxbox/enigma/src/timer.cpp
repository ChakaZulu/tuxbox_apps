#include "timer.h"
#include <core/dvb/dvbservice.h>

void eRecordingTimer::timeout()
{
	time_t now=time(0)+eDVB::getInstance()->time_difference;
	eDebug("[TIMER] event start, now: %d", now);
	
	if (now < (start_time-5))
	{
		time_t stime=start_time - time(0)+eDVB::getInstance()->time_difference - 5;
		eDebug("[TIMER] not yet, restarting timer to %d...", stime);
		timer.start( stime );
	} else if (now < start_time+duration)
	{
		if (state == stateWaiting)
		{
			eDebug("[TIMER] making ready");
			if (ready())
				eDebug("[TIMER] ready failed!");
		}
		
		if (state == stateReady)
		{
			if (now >= start_time)
			{
				eDebug("start!");
				start();
			}
			timer.start(start_time-now+duration);
		} else if (state == stateRunning)
		{
			eDebug("[TIMER] spurious timer");
			timer.start(start_time-now+duration);
		} else
			eDebug("[TIMER] not ready?!");
	} else
	{
		if (state != stateDone)
			abort();
	}
}

void eRecordingTimer::rec_ready()
{
	eDebug("[TIMER] ready()");
	switch (action)
	{
	case actionRecord:
	{
#if 0
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	
		if (!sapi)
		{
			status=statusFailure;
			return;
		}
	
		sapi->switchService(service);
	
		ASSERT(!recorder);
	
		recorder=new eDVBRecorder();
		recorder->open(filename.c_str());
		break;
#endif
	}
	default:
		break;
	}
}

void eRecordingTimer::rec_start()
{
	eDebug("[TIMER] start()");
	switch (action)
	{
	case actionRecord:
		ASSERT(recorder);
		recorder->start();
		break;
	default:
		break;
	}
}

void eRecordingTimer::rec_pause()
{
	eDebug("[TIMER] pause()");
	switch (action)
	{
	case actionRecord:
		ASSERT(recorder);
		recorder->stop();
		break;
	default:
		break;
	}
}

void eRecordingTimer::rec_stop()
{
	eDebug("[TIMER] stop()");
	switch (action)
	{
	case actionRecord:
		ASSERT(recorder);
		recorder->stop();
		recorder->close();
		delete recorder;
		recorder=0;
		break;
	default:
		break;
	}
}

void eRecordingTimer::serviceChanged(const eServiceReference &service, int err)
{
	if ((state == stateReady) || (state == statePause) || (state == stateRunning))
	{
		if (err)
		{
			status=statusFailure;
			return;
		}
	}
}

void eRecordingTimer::gotPMT(PMT *pmt)
{
	switch (state)
	{
	case stateRunning:
		rec_stop();
	case stateReady:
	case statePause:
		if (action == actionRecord)
		{
			ASSERT(recorder);
			recorder->removeAllPIDs();
			for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
				recorder->addPID(i->elementary_PID);
			recorder->addPID(0); // PAT
			recorder->addPID(12); // EIT
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	
			if (sapi)
				recorder->addPID(sapi->pmtpid);
			if (state == stateRunning)
				rec_start();
		}
		break;
	default:
		break;
	}
}

eRecordingTimer::eRecordingTimer()
		: timer(eApp), start_time(0), recorder(0)
{
//	CONNECT(eDVB::getInstance()->switchedService, eRecordingTimer::serviceChanged);
	CONNECT(eDVB::getInstance()->gotPMT, eRecordingTimer::gotPMT);
	CONNECT(timer.timeout, eRecordingTimer::timeout);
	state=stateDisabled;
}

eRecordingTimer::~eRecordingTimer()
{
	abort();
	ASSERT(!recorder);
}

int eRecordingTimer::disable()
{
	if (state != stateWaiting)
		return -1;
	timer.stop();
	state = stateDisabled;
	return 0;
}

int eRecordingTimer::enable()
{
	if (state != stateDisabled)
		return -1;
	time_t stime=start_time - time(0)+eDVB::getInstance()->time_difference - 5;
	timer.start( stime );
	eDebug("[TIMER] enabled at %d", stime);
	state = stateWaiting;
	return 0;
}

int eRecordingTimer::ready()
{
	if ((state != stateDisabled) && (state != stateWaiting) && (state != stateDone))
		return -1;
	rec_ready();
	state = stateReady;
	return 0;
}

int eRecordingTimer::start()
{
	if (state != stateReady)
		if (ready())
			return -1;
	rec_start();
	return 0;
}

int eRecordingTimer::pause()
{
	if (state != stateRunning)
		return -1;
	rec_pause();
	state = statePause;
	return 0;
}

int eRecordingTimer::resume()
{
	if (state != statePause)
		return -1;
	rec_start();
	state = stateRunning;
	return 0;
}

int eRecordingTimer::abort()
{
	if (state == stateRunning)
		if (pause())
			return -1;
	if ((state != stateWaiting) && (state != statePause))
		return -1;
	if (state == statePause)
		rec_stop();
	state = stateDone;
	return 0;
}

eTimerManager::eTimerManager()
{
#if 0
	recordtimer.setAutoDelete(true);
	
	eRecordingTimer *n=new eRecordingTimer();
	recordtimer.push_back(n);
	n->start_time=time(0)+eDVB::getInstance()->time_difference + 60;
	n->service=eServiceReference(eTransportStreamID(0x437), eOriginalNetworkID(0x0001), eServiceID(0x6d66), 1);
	n->enable();
#endif
}

eTimerManager::~eTimerManager()
{
}
