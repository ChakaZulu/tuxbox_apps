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

class eDVBServiceController: public eDVBController, public Object
{
	Signal0<void> freeCheckFinishedCallback;
	void freeCheckFinished();
public:
		/* current service */
	eServiceReferenceDVB service,  // meta-service
			     parentservice,prevservice;	// for linkage handling
	eTransponder *transponder;
	int pmtpid,
			service_state;
	MHWEIT *tMHWEIT;
	TDT *tdt;

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
#endif

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
