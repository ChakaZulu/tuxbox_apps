#ifndef __src_core_dvb_dvbservice_h
#define __src_core_dvb_dvbservice_h

#include <core/dvb/edvb.h>

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

class eDVBServiceController: public eDVBController, public Object
{
public:
		/* currently tuned in */
	eTransponder *currentTransponder;
	int currentTransponderState;
		/* current service */
	eServiceReference service;	// meta-service
	eTransponder *transponder;
	int pmtpid;
	int service_state;
	MHWEIT *tMHWEIT;
	TDT *tdt;

//	void MHWEITready(int error);

	struct CA
	{
		int casysid, ecmpid, emmpid;
	};
	
	std::list<int> availableCASystems;
	ePtrList<CA> calist;		/** currently used ca-systems */
	
	int checkCA(ePtrList<CA> &list, const ePtrList<Descriptor> &descriptors);

	void scanPMT();

	void PATready(int error);
	void SDTready(int error);
	void PMTready(int error);
	void NITready(int error);
	void ONITready(int error);
	void EITready(int error);
	void TDTready(int error);
	void BATready(int error);
	void MHWEITready(int error);

	void setPID(PMTEntry *entry);
	void setDecoder();

	eDVBServiceController(eDVB &dvb);
	~eDVBServiceController();
	void handleEvent(const eDVBEvent &event);

	int switchService(const eServiceReference &service); /** -> eventServiceSwitched */
};

#endif
