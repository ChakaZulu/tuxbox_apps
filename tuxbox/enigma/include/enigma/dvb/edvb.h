#ifndef __edvb_h
#define __edvb_h

#include "qobject.h"
#include "qlist.h"
#include "esection.h"
#include <stdio.h>
#include "epgcache.h"
#include "nconfig.h"

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

/*
  eDVB contains high-level dvb-functions like 
  "switchService" which can be called almost 
  stateless from everywhere...
 */

#define ENOCASYS	1000	/** service is not free and no valid caid */
#define ENOSTREAM 1001  /** no video or audio stream */
#define ENVOD			1002	/** nvod stream has to be selected */

#define SCAN_ONIT	1			/** scan network_other NIT */
#define SCAN_SKIP	2			/** skip known NITs (assuming they're all the same) */

class eBouquet;
class eAVSwitch;
class eStreamWatchdog;

class eDVB: public QObject
{
	Q_OBJECT
	static eDVB *instance;
protected:
		/** the main transponder/servicelist */
	eTransponderList *transponderlist;
	
	QList<eBouquet> bouquets;
	void removeDVBBouquets();
	void addDVBBouquet(BAT *bat);
	eBouquet *getBouquet(int bouquet_id);
	eBouquet *getBouquet(QString bouquet_name);
	eBouquet *createBouquet(int bouquet_id, QString bouquet_name);
	eBouquet *createBouquet(QString bouquet_name);
	int getUnusedBouquetID(int range);
	
	void revalidateBouquets();

		/** the current transponder with errorcode */
	eTransponder *currentTransponder;
	const QList<eTransponder> *initialTransponders;
	int currentTransponderState;

		/** tables for current service/transponder */
	eAUTable<PAT> tPAT;
	eAUTable<PMT> tPMT;
	eAUTable<SDT> tSDT;
	eAUTable<NIT> tNIT, tONIT;
	eAUTable<EIT> tEIT;
	eAUTable<BAT> tBAT;
	
	TDT *tdt;
	
	eEPGCache epgcache;
	eStreamWatchdog *streamwd;
	
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
	
	QList<int> availableCASystems;
	QList<CA> calist;		/** currently used ca-systems */
	
	int time_difference;

protected:

	int checkCA(QList<CA> &list, const QList<Descriptor> &descriptors);
		/* SCAN internal */
	int scanOK;	// 1 SDT, 2 NIT, 4 BAT, 8 oNIT
	int currentONID, scanflags;
	eTransponder *scannedTransponder;
	void scanEvent(int event);
	QList<int> knownNetworks;

		/* SWITCH internal */

	void serviceEvent(int event);	
	void scanPMT();

private slots:
	void tunedIn(eTransponder*, int);
	
	void PATready(int error);
	void SDTready(int error);
	void PMTready(int error);
	void NITready(int error);
	void ONITready(int error);
	void EITready(int error);
	void TDTready(int error);
	void BATready(int error);

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
	void setState(int newstate) { emit stateChanged(state=newstate); }

		/* generic state - public */
signals:
	void stateChanged(int newstate);
	void eventOccured(int event);

		/* public general signals */
signals:
	void serviceListChanged();
	void bouquetListChanged();
	void leaveService(eService *);
	void enterService(eService *);	/** only succesfull channel-switches */

	void leaveTransponder(eTransponder *);
	void enterTransponder(eTransponder *);
	
	void switchedTransponder(eTransponder*,int);
	void switchedService(eService*,int);
	
	void gotEIT(EIT *eit, int);
	void gotSDT(SDT *sdt);
	void gotPMT(PMT *pmt);
	void scrambled(bool);

		/* SCAN - public */
public:
	int startScan(const QList<eTransponder> &initital, int flags);	/** -> stateScanComplete */

		/* SERVICE SWITCH - public */
	int switchService(eService *service); /** -> eventServiceSwitched */
	int switchService(int nservice_id, int noriginal_network_id, int ntransport_stream_id, int nservice_type); /** -> stateServiceSwitched */

public:
	QString getVersion();
	eDVB();
	~eDVB();
	eTransponderList *getTransponders();
	QList<eService> *getServices();
	QList<eBouquet> *getBouquets();
	static eDVB *getInstance()
	{
		return instance;
	}
	void setTransponders(eTransponderList *tlist);

	QString getInfo(const char *info);
	
	void setPID(PMTEntry *entry);
	void setDecoder();
	PMT *getPMT(); 
	EIT *getEIT();
	
	void sortInChannels();

	void saveServices();
	void loadServices();

	void saveBouquets();
	void loadBouquets();
	
	int useAC3, useBAT;

	int volume, mute;	
	void changeVolume(int abs, int vol);		// vol: 0..63, 63 is MIN; abs=0/1 vol, 2/3 mute
	
	NConfig config;
	
	void configureNetwork();

signals:
	void volumeChanged(int vol);
};

#endif
