#ifndef __edvb_h
#define __edvb_h

#include "esection.h"
#include <stdio.h>
#include "nconfig.h"
#include <list>
#include <string>
#include <eptrlist.h>

#include <sigc++/signal_system.h>
#ifdef SIGC_CXX_NAMESPACES
using namespace SigC;
#endif

class eService;
class eTransponder;
class eTransponderList;

class PAT;
class PMT;
class PMTEntry;
class SDT;
class NIT;
class EIT;
class TDT;
class BAT;
class Descriptor;

#define ENOCASYS	1000	/// service is not free and no valid caid
#define ENOSTREAM 1001  /// no video or audio stream
#define ENVOD			1002	/// nvod stream has to be selected

#define SCAN_ONIT	1			/// scan network_other NIT
#define SCAN_SKIP	2			/// skip known NITs (assuming they're all the same)

class eBouquet;
class eAVSwitch;
class eStreamWatchdog;
class MHWEIT;
class eDVBRecorder;

/**
 * \brief High level DVB class.
 *
 * eDVB contains high-level dvb-functions like 
 * "switchService" which can be called almost 
 * stateless from everywhere...
 */
class eDVB: public Object
{
	static eDVB *instance;
protected:
	friend class sortinChannel;
		/** the main transponder/servicelist */
	eTransponderList *transponderlist;
	
	ePtrList<eBouquet> bouquets;
	void removeDVBBouquets();
	void addDVBBouquet(BAT *bat);
	eBouquet *getBouquet(int bouquet_id);
	eBouquet *getBouquet(std::string bouquet_name);
	eBouquet *createBouquet(const eBouquet *parent, int bouquet_id, std::string bouquet_name);
	eBouquet *createBouquet(const eBouquet *parent, std::string bouquet_name);
	int getUnusedBouquetID(int range);
	
	void revalidateBouquets();

		/** the current transponder with errorcode */
	eTransponder *currentTransponder;
	const ePtrList<eTransponder> *initialTransponders;
	int currentTransponderState;

		/** tables for current service/transponder */
	eAUTable<PAT> tPAT;
	eAUTable<PMT> tPMT;
	eAUTable<SDT> tSDT;
	eAUTable<NIT> tNIT, tONIT;
	eAUTable<EIT> tEIT;
	eAUTable<BAT> tBAT;
	MHWEIT *tMHWEIT;
	
	TDT *tdt;
	
	eDVBRecorder *recorder;
	
public:
		/** current service */
	eService *service;	// meta-service
	eTransponder *transponder;
	int original_network_id, transport_stream_id, service_id, service_type;	// tunedIn only in idle-state. raw services.
	int pmtpid;
	int service_state;

	struct CA
	{
		int casysid, ecmpid, emmpid;
	};
	
	std::list<int> availableCASystems;
	ePtrList<CA> calist;		/** currently used ca-systems */
	
	int time_difference;

protected:

	int checkCA(ePtrList<CA> &list, const ePtrList<Descriptor> &descriptors);
		/* SCAN internal */
	int scanOK;	// 1 SDT, 2 NIT, 4 BAT, 8 oNIT
	int currentONID, scanflags;
	eTransponder *scannedTransponder;
	void scanEvent(int event);
	std::list<int> knownNetworks;

		/* SWITCH internal */

	void serviceEvent(int event);	
	void scanPMT();

private:// slots:
	void tunedIn(eTransponder*, int);
	void PATready(int error);
	void SDTready(int error);
	void PMTready(int error);
	void NITready(int error);
	void ONITready(int error);
	void EITready(int error);
	void TDTready(int error);
	void BATready(int error);
	void MHWEITready(int error);

public:
	enum
	{
		stateIdle,
		eventScanBegin,		// -> next
		eventScanNext,		// -> tune
		stateScanTune,		// tune ended mit "tuned" in switchedTransponder
		eventScanTuneOK,	// tuneOK führt zu "getPAT"
		eventScanTuneError,	// tuneError führt zu ScanError
		stateScanGetPAT,	// -> gotPAT:scanError (PATready)
		eventScanGotPAT,	// -> Wait
		stateScanWait,
		eventScanGotSDT,	// scanOK |= SDT
		eventScanGotNIT,	// scanOK |= NIT
		eventScanGotONIT,	// scanOK |= ONIT
		eventScanGotBAT,	// scanOK |= BAT
		eventScanComplete,
		eventScanError,
		stateScanComplete,
		eventScanCompleted,
		
		eventServiceSwitch,		// -> eventServiceSwitched or eventServiceFailed
		stateServiceTune,
		eventServiceTuneOK,
		eventServiceTuneFailed,	
		stateServiceGetPAT,
		eventServiceGotPAT,
		stateServiceGetPMT,
		eventServiceGotPMT,
		eventServiceNewPIDs,
		
		stateServiceGetSDT,
		eventServiceGotSDT,

		eventServiceSwitched,
		eventServiceFailed,
	};

private:
	int state;
	void setState(int newstate) { /*emit*/ stateChanged(state=newstate); }

public:
	Signal1<void, int> stateChanged;
	Signal1<void, int> eventOccured;
	Signal0<void> serviceListChanged;
	Signal0<void> bouquetListChanged;
	Signal1<void, eService*> leaveService;
	Signal1<void, eService*> enterService;
	Signal1<void, eTransponder*> leaveTransponder;
	Signal1<void, eTransponder*> enterTransponder;
	Signal2<void, eTransponder*, int> switchedTransponder;
	Signal2<void, eService*, int> switchedService;
	Signal2<void, EIT*, int> gotEIT;
	Signal1<void, SDT*> gotSDT;
	Signal1<void, PMT*> gotPMT;
	Signal1<void, bool> scrambled;
	Signal1<void, int> volumeChanged;
	Signal0<void> timeUpdated;
		/* SCAN - public */
public:
	int startScan(const ePtrList<eTransponder> &initital, int flags);	/** -> stateScanComplete */

		/* SERVICE SWITCH - public */
	int switchService(eService *service); /** -> eventServiceSwitched */
	int switchService(int nservice_id, int noriginal_network_id, int ntransport_stream_id, int nservice_type); /** -> stateServiceSwitched */

public:
	std::string getVersion();
	eDVB();
	~eDVB();
	eTransponderList *getTransponders();
	ePtrList<eBouquet> *getBouquets();
	static eDVB *getInstance()
	{
		return instance;
	}
	void setTransponders(eTransponderList *tlist);

	std::string getInfo(const char *info);
	
	void setPID(PMTEntry *entry);
	void setDecoder();

	PMT *getPMT();
	EIT *getEIT();
	
	void sortInChannels();

	void saveServices();
	void loadServices();

	void saveBouquets();
	void loadBouquets();
	
	int useBAT;

	int volume, mute;	
	/**
	 * \brief Changes the volume.
	 *
	 * \param abs What to change:
	 * \arg \c 0 Volume, relative
	 * \arg \c 1 Volume, absolute
	 * \arg \c 2 Mute, set
	 * \arg \c 3 Mute, change
	 * \param vol The volume/muteflag to set. In case of volume, 0 means max and 63 means min.
	 */
	void changeVolume(int abs, int vol);
	
	/**
	 * \brief Configures the network.
	 *
	 * Configures the network according to the configuration stored in the registry.
	 */
	void configureNetwork();
	
		/// starts a new recording
	void recBegin(const char *filename); 
		/// pauses a recording
	void recPause();
		/// resumes a recording
	void recResume();
		/// closes a recording
	void recEnd();
};

#endif
