#include "timer.h"
#include <core/dvb/dvbservice.h>

void eRecordingTimer::ready()
{
	eDebug("[TIMER] ready()");
	switch (action)
	{
	case actionRecord:
	{
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
	}
	default:
		break;
	}
}

void eRecordingTimer::start()
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

void eRecordingTimer::pause()
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

void eRecordingTimer::stop()
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

void eRecordingTimer::setState(int newstate)
{
	switch (newstate)
	{
	case stateReady:
		switch (state)
		{
		case stateRunning:
			setState(statePause);
		case statePause:
		case stateDisabled:
		case stateWaiting:
		case stateDone:
			ready();
			break;
		}
		break;
	case statePause:
		switch (state)
		{
		case stateDisabled:
		case stateDone:
		case stateWaiting:
			ready();
		case stateReady:
			start();
		case stateRunning:
			pause();
			break;
		}
		state=statePause;
		break;
	case stateRunning:
		switch (state)
		{
		case stateDone:
		case stateDisabled:
		case stateWaiting:
			setState(stateReady);
		case statePause:
		case stateReady:
			start();
			break;
		case stateRunning:
			break;
		}
		state=stateRunning;
		break;
	case stateDisabled:
	case stateWaiting:
	case stateDone:
		switch (state)
		{
		case stateRunning:
		case statePause:
		case stateReady:
			stop();
		case stateDisabled:
		case stateWaiting:
			state=stateDone;
			break;
		}
		break;
	}
	
	state=newstate;
}

void eRecordingTimer::gotPMT(PMT *pmt)
{
	switch (state)
	{
	case stateRunning:
		stop();
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
				start();
		}
		break;
	default:
		break;
	}
}

eRecordingTimer::eRecordingTimer(time_t starttime)
		: start_time(starttime), recorder(0)
{
	CONNECT(eDVB::getInstance()->switchedService, eRecordingTimer::serviceChanged);
	CONNECT(eDVB::getInstance()->gotPMT, eRecordingTimer::gotPMT);
}

eRecordingTimer::~eRecordingTimer()
{
	setState(stateDone);
	ASSERT(!recorder);
}

#if 0

void eTimerManager::process()
{
	int ttgr=5; // time to go ready (5s)
	int now=time(0)+eDVB::getInstance()->time_difference;
	for (timerList::iterator i(recordtimer.begin()); i != recordtimer.end(); ++i)
	{
		if ((i->state == stateDone) || (i->state == stateWaiting))
			continue;
		if ((i->start_time - ttgr) > now)
		{
			int delay=i->start_time - ttgr;
		}
		
	}
}
#endif

eTimerManager::eTimerManager(): timer(eApp)
{
}

eTimerManager::~eTimerManager()
{
}
