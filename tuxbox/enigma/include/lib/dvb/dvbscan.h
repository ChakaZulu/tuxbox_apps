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
			// not compatible to xml-flags!
		flagUseONIT=1,
		flagUseBAT=2,
		flagNetworkSearch=4,
		flagSkipKnownNIT=8,
		flagClearList=16,
		flagSkipOtherOrbitalPositions=32
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
	std::list<eTransponder> changedTransponder;

	int handleSDT(eTransponder *&transponder, const SDT *sdt);

public:
	eDVBScanController(eDVB &dvb);
	~eDVBScanController();

	void handleEvent(const eDVBEvent &event);
	
	bool addTransponder(const eTransponder &transponder)
	{
//		eDebug("TP TO ADD -> freq = %i, srate = %i, pol = %i, fec = %i, svalid = %i, cvalid = %i, onid = %i, tsid = %i, inv = %i, op = %i",transponder.satellite.frequency, transponder.satellite.symbol_rate, transponder.satellite.polarisation, transponder.satellite.fec,  transponder.satellite.valid, transponder.cable.valid, transponder.original_network_id.get(), transponder.transport_stream_id.get(), transponder.satellite.inversion, transponder.satellite.orbital_position);

		if ( transponder.satellite.orbital_position != knownTransponder.front().satellite.orbital_position && flags & flagSkipOtherOrbitalPositions )
//		{
//			eDebug("Skip Transponder from other orbital position");
			return false;
//		}

		for ( std::list<eTransponder>::iterator n(changedTransponder.begin()); n != changedTransponder.end(); ++n)
//		{
			if (*n == transponder)
				return false;
//		}

		for (std::list<eTransponder>::iterator n(knownTransponder.begin()); n != knownTransponder.end(); ++n)
//		{
//			eDebug("COMPARE WITH -> freq = %i, srate = %i, pol = %i, fec = %i, svalid = %i, cvalid = %i, onid = %i, tsid = %i, inv = %i, op = %i",n->satellite.frequency, n->satellite.symbol_rate, n->satellite.polarisation, n->satellite.fec,  n->satellite.valid, n->cable.valid, n->original_network_id.get(), n->transport_stream_id.get(), n->satellite.inversion, n->satellite.orbital_position);
			if (*n == transponder)  // no dupe Transponders
//			{
	//			eDebug("Transponder is already in list");
				return false;
//			}
//	}
//		eDebug("Transponder added");

		eTransponder t=transponder;
		t.state=eTransponder::stateToScan;
		knownTransponder.push_back(t);
		return true;
	}

	int getknownTransponderSize()	{ return knownTransponder.size(); }
	
	void setUseONIT(int useonit);
	void setUseBAT(int usebat);
	void setNetworkSearch(int networksearch);
	void setClearList(int clearlist);
	void setSkipKnownNIT(int skip);
	void setSkipOtherOrbitalPositions(int skipOtherOP);
	
	void start();
};

#endif
