#ifndef __src_core_dvb_dvbscan_h
#define __src_core_dvb_dvbscan_h

#include <core/dvb/edvb.h>

#include <set>
	
class eDVBScanEvent: public eDVBEvent
{
public:
	enum
	{
		eventScanBegin=eDVBEvent::eventUser,		// -> next
		eventScanTPadded,
		eventScanNext,		// -> tune
		eventScanTuneOK,	// tuneOK führt zu "getPAT"
		eventScanTuneError,	// tuneError führt zu ScanError
		eventScanGotPAT,	// -> Wait
		eventScanGotSDT,	// scanOK |= SDT
		eventScanGotNIT,	// scanOK |= NIT
		eventScanGotONIT,	// scanOK |= ONIT
		eventScanGotBAT,	// scanOK |= BAT
		eventScanComplete,
		eventScanError,
		eventScanCompleted
	};
	eDVBScanEvent(int event): eDVBEvent(event) { }
	eDVBScanEvent(int event, int err, eTransponder *transponder): eDVBEvent(event, err, transponder) { }
};

class eDVBScanState: public eDVBState
{
public:
	enum serviceEvent
	{
		stateScanTune=eDVBState::stateUser,		// tune ended mit "tuned" in switchedTransponder
		stateScanGetPAT,	// -> gotPAT:scanError (PATready)
		stateScanWait,
		stateScanComplete,
	};
	eDVBScanState(int state): eDVBState(state) { }
};

class eDVBScanController: public eDVBController, public Object
{
	int flags;
	
	enum
	{
		flagUseONIT=1,
		flagUseBAT=2,
		flagNetworkSearch=4,
		flagClearList=8
	};
	
	int scanOK;	// 1 SDT, 2 NIT, 4 BAT, 8 oNIT
	int currentONID, scanflags;
			// der aktuelle gescannte transponder
	eTransponder *transponder;
	std::set<int> knownNetworks;

	void PATready(int error);
	void SDTready(int error);
	void NITready(int error);
	void ONITready(int error);
	void BATready(int error);
	
	std::list<eTransponder> knownTransponder;

	int handleSDT(eTransponder *&transponder, const SDT *sdt);

public:
	eDVBScanController(eDVB &dvb);
	~eDVBScanController();

	void handleEvent(const eDVBEvent &event);
	
	void addTransponder(const eTransponder &transponder)
	{
		eTransponder t=transponder;
		t.state=eTransponder::stateToScan;
		knownTransponder.push_back(t);
		
	}

	int getknownTransponderSize()	{ return knownTransponder.size(); }
	
	void setUseONIT(int useonit);
	void setUseBAT(int usebat);
	void setNetworkSearch(int networksearch);
	void setClearList(int clearlist);
	
	void start();
};

#endif
