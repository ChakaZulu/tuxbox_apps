#ifndef __lib_dvb_service_h
#define __lib_dvb_service_h

#include <map>
#include <libsig_comp.h>
#include <lib/dvb/dvb.h>

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

class eServiceCommand
{
public:
	eServiceCommand(int type): type(type) { }
	eServiceCommand(int type, int parm): type(type), parm(parm) { }
	enum
	{
		cmdRecordOpen,
		cmdRecordStart,
		cmdRecordStop,
		cmdRecordClose,

		cmdSeekBegin,
		cmdSeekEnd,
		cmdSetSpeed,		// parm : ratio.. 1 normal, 0 pause, >1 fast forward, <0 reverse (if supported)
		cmdSkip,				// parm : in ms (~)
		cmdSeekAbsolute,	// parm : percentage ~
		cmdSeekReal			// parm : service specific, as given by queryRealPosition
	};
	int type;
	int parm;
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
		flagIsSeekable=8,
		flagSupportPosition=16
	};
	enum
	{
		statePlaying, stateError, stateScrambled, stateStopped, statePause, stateSkipping
	};
	eServiceHandler(int id);
	int getID()
	{
		return id;
	}
	virtual ~eServiceHandler();
	virtual eService *createService(const eServiceReference &node);

	virtual int play(const eServiceReference &service);

		// current service
	virtual int serviceCommand(const eServiceCommand &cmd);

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
	
		// position query
	enum {
		posQueryLength,	// query length (in seconds)
		posQueryCurrent, // query current position
		posQueryRealLength, // service specific length, e.g. file length in bytes
		posQueryRealCurrent // service specific current position, e.g. file position in bytes
	};
	virtual int getPosition(int what);	// -1 means: not available

	Signal1<void, const eServiceEvent &> serviceEvent;

		// service list functions
	virtual void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	virtual void leaveDirectory(const eServiceReference &dir);

	virtual eService *addRef(const eServiceReference &service);
	virtual void removeRef(const eServiceReference &service);
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
	
		// stuff for modifiying ...

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);
};

#endif
