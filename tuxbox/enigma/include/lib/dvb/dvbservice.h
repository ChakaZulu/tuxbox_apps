#ifndef __src_lib_dvb_dvbservice_h
#define __src_lib_dvb_dvbservice_h

#include <lib/dvb/edvb.h>

#ifndef DISABLE_CI
class eDVBCI;
#endif

class eDVBServiceEvent: public eDVBEvent
{
public:
	enum
	{
		eventServiceSwitch=eDVBEvent::eventUser,		// -> eventServiceSwitched or eventServiceFailed
		eventServiceTuneOK,
		eventServiceTuneFailed,	
		eventServiceGotPAT,
		eventServiceGotPMT,
		eventServiceNewPIDs,
		
		eventServiceGotSDT,

		eventServiceSwitched,
		eventServiceFailed,
	};
	eDVBServiceEvent(int event): eDVBEvent(event) { }
	eDVBServiceEvent(int event, int err, eTransponder *transponder): eDVBEvent(event, err, transponder) { }
};

class eDVBServiceState: public eDVBState
{
public:
	enum serviceEvent
	{
		stateServiceTune=eDVBState::stateUser,
		stateServiceGetPAT,
		stateServiceGetPMT,
		stateServiceGetSDT,
	};
	eDVBServiceState(int state): eDVBState(state) { }
};

class eDVBCaPMTClient
{
	int lastPMTVersion;
public:
	virtual void handlePMT(const eServiceReferenceDVB &, PMT *pmt) { }
	virtual void enterService(const eServiceReferenceDVB &) { }
	virtual void leaveService(const eServiceReferenceDVB &) { }
};

class eDVBCaPMTClientHandler
{
	static std::set<eDVBCaPMTClient*> capmtclients;
public:
	static void registerCaPMTClient( eDVBCaPMTClient* cl )
	{
		capmtclients.insert( cl );
	}

	static void unregisterCaPMTClient( eDVBCaPMTClient* cl )
	{
		capmtclients.erase( cl );
	}

	static void distribute_enterService( const eServiceReferenceDVB &service )
	{
		for ( std::set<eDVBCaPMTClient*>::iterator it(capmtclients.begin());
			it != capmtclients.end(); ++it )
		(*it)->enterService(service);
	}

	static void distribute_leaveService( const eServiceReferenceDVB &service )
	{
		for ( std::set<eDVBCaPMTClient*>::iterator it(capmtclients.begin());
			it != capmtclients.end(); ++it )
		(*it)->leaveService(service);
	}

	static void distribute_gotPMT(const eServiceReferenceDVB &service, PMT *pmt)
	{
		for ( std::set<eDVBCaPMTClient*>::iterator it(capmtclients.begin());
			it != capmtclients.end(); ++it )
		(*it)->handlePMT(service,pmt);
	}
};

class eDVBServiceController
	:public eDVBController, public eDVBCaPMTClient, public Object
{
	Signal0<void> freeCheckFinishedCallback;
	void freeCheckFinished();
	int lastPMTVersion;
public:
		/* current service */
	eServiceReferenceDVB service,  // meta-service
			     parentservice,prevservice;	// for linkage handling
	eTransponder *transponder;
	int pmtpid,
			service_state;
	MHWEIT *tMHWEIT;
	TDT *tdt;

	static eLock availCALock;

	struct CA
	{
		int casysid, ecmpid, emmpid;
	};

	std::set<int> availableCASystems, usedCASystems;
	ePtrList<CA> calist;		/** currently used ca-systems */

	int checkCA(ePtrList<CA> &list, const ePtrList<Descriptor> &descriptors, int sid);

#ifndef DISABLE_CI
	eDVBCI *DVBCI;
	eDVBCI *DVBCI2;
	// for CI handling
	void handlePMT(const eServiceReferenceDVB &, PMT*);
#endif

	// set pids... detect used ca systems
	void scanPMT( PMT *pmt );

	void PATready(int error);
	void SDTready(int error);
	void PMTready(int error);
	void NITready(int error);
	void ONITready(int error);
	void EITready(int error);
	void TDTready(int error);
	void BATready(int error);
	void MHWEITready(int error);

	void setPID(const PMTEntry *entry);
	void setDecoder();

	eDVBServiceController(eDVB &dvb);
	~eDVBServiceController();
	void handleEvent(const eDVBEvent &event);

	int switchService(const eServiceReferenceDVB &service); /** -> eventServiceSwitched */

	void initCAlist();
	void clearCAlist();
};

#endif
