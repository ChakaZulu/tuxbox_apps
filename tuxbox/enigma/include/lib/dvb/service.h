#ifndef __core_dvb_service_h
#define __core_dvb_service_h

/*
	muss können:
	
	switchService (OOH)
	SubService-Verwaltung (uh oh)
	Audiochannels benennen (laut EIT wg. componentTag)
	Audiochannel wechseln
	SubService-Wechseln (no prob)
	Flags abfragen: Scrambled? Key Available? neue EIT? 
	change volume
	EIT list abfragen
	ordentlichen namen vom service kriegen

*/

#include <map>
#include <include/libsig_comp.h>
#include <core/dvb/dvb.h>

class eServiceReference;

class eServiceEvent
{
public:
	eServiceEvent(int type, void* data=0): type(type), data(data)
	{
	}
	enum
	{
		evtStop, 
		evtStart, 
		evtPause, 
		
		evtGotPMT,
		evtGotSDT,
		evtGotEIT,
		
		evtStateChanged,
		evtFlagsChanged,
		
		evtAspectChanged		
	};
	int type;
	void* data;
};

class PMTEntry;
class eService;
class EIT;
class PMT;
class SDT;
class PMTEntry;

class eServiceHandler: public Object
{
protected:
	int id;
public:
	enum
	{
		flagIsScrambled=1, 
		flagHaveSubservices=2, 
		flagHaveMultipleAudioStreams=4,
	};
	enum
	{
		statePlaying, stateError, stateScrambled
	};
	eServiceHandler(int id);
	virtual int getID() const =0;
	virtual ~eServiceHandler()=0;
	virtual eService *lookupService(const eServiceReference &service)=0;

	virtual int play(const eServiceReference &service)=0;

		// current service

		// for DVB audio channels:
	virtual PMT *getPMT()=0;
	virtual void setPID(const PMTEntry *)=0;
	
		// for DVB nvod channels:
	virtual SDT *getSDT()=0;

		// for DVB events, nvod, audio....
	virtual EIT *getEIT()=0;
	
	virtual int getFlags()=0;
	virtual int getState()=0;
	
		// get visual flags
	virtual int getAspectRatio()=0;
	
	virtual int stop()=0;

	Signal1<void, const eServiceEvent &> serviceEvent;
};

class eService;

class eServiceInterface: public Object
{
	eServiceHandler *currentServiceHandler;
	std::map<int,eServiceHandler*> handlers;
	int switchServiceHandler(int id);
	
	SigC::Connection conn;
	void handleServiceEvent(const eServiceEvent &event);
	
	static eServiceInterface *instance;
public:
	eServiceInterface();
	~eServiceInterface();
	static eServiceInterface *getInstance();

	int registerHandler(int id, eServiceHandler *handler);
	int unregisterHandler(int id);
	eServiceHandler *getServiceHandler(int id);

	eService *lookupService(const eServiceReference &service);

	int play(const eServiceReference &service);
	
		// service related functions
	
	Signal1<void,const eServiceEvent &> serviceEvent;
	
	eServiceHandler *getService()
	{
		return currentServiceHandler;
	}
	
	int stop();

	eServiceReference service;
};

#endif
