#ifndef __core_dvb_service_h
#define __core_dvb_service_h

#include <map>
#include <include/libsig_comp.h>
#include <core/dvb/dvb.h>

class eServiceReference;

class eServiceEvent
{
public:
	eServiceEvent(int type): type(type)
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
		
		evtAspectChanged,
		
		evtEnd
	};
	int type;
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
		statePlaying, stateError, stateScrambled, stateStopped
	};
	eServiceHandler(int id);
	int getID()
	{
		return id;
	}
	virtual ~eServiceHandler();
	virtual eService *lookupService(const eServiceReference &service)=0;

	virtual eService *createService(const eServiceReference &node);

	virtual int play(const eServiceReference &service);

		// current service

		// for DVB audio channels:
	virtual PMT *getPMT();
	virtual void setPID(const PMTEntry *);
	
		// for DVB nvod channels:
	virtual SDT *getSDT();

		// for DVB events, nvod, audio....
	virtual EIT *getEIT();
	
	virtual int getFlags();
	virtual int getState();
	
		// get visual flags
	virtual int getAspectRatio();

	virtual int getErrorInfo();
	
	virtual int stop();

	Signal1<void, const eServiceEvent &> serviceEvent;

		// service list functions
	virtual void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	virtual void leaveDirectory(const eServiceReference &dir);
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
		
		// service list functions
	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);
};

#endif
