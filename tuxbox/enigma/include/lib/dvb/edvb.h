#ifndef __edvb_h
#define __edvb_h

#include <stdio.h>
#include <list>

#include <libsig_comp.h>
#include <lib/dvb/esection.h>
#include <lib/system/econfig.h>
#include <lib/base/estring.h>
#include <lib/base/eptrlist.h>
#include <lib/dvb/settings.h>

class eService;
class eTransponder;
class eTransponderList;
class eDVBServiceController;

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

class eBouquet;
class eAVSwitch;
class eStreamWatchdog;
class eRFmod;
class MHWEIT;
class eDVBRecorder;
class eDVBScanController;
class eDVB;

class eTransponder;

#ifndef DISABLE_CI
class eDVBCI;
#endif

class eDVBEvent
{
public:
	int type;
	enum
	{
		eventTunedIn,
		eventRecordWriteError,
		eventUser,
	};
	int err;
	eTransponder *transponder;
	
	eDVBEvent(int type): type(type) { }
	eDVBEvent(int type, int err, eTransponder *transponder): type(type), err(err), transponder(transponder) { }
};

class eDVBState
{
public:
	int state;
	enum
	{
		stateIdle,
		stateUser
	};
	eDVBState(int state): state(state) { }
	operator int () const { return state; }
};

class eDVBController
{
protected:
	eDVB &dvb;
public:
	eDVBController(eDVB &dvb): dvb(dvb) { }
	virtual ~eDVBController()=0;
	virtual void handleEvent(const eDVBEvent &event)=0;
};

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

#ifndef DISABLE_FILE
	void recMessage(int);
#endif // DISABLE_FILE
public:
		/** tables for current service/transponder */
	eAUTable<PAT> tPAT;
	eAUTable<PMT> tPMT;
	eAUTable<SDT> tSDT;
	eAUTable<NIT> tNIT, tONIT;
	eAUTable<EIT> tEIT;
	eAUTable<BAT> tBAT;
	EIT *parentEIT;

	eDVBRecorder *recorder;
#ifndef DISABLE_CI
	eDVBCI *DVBCI;
	eDVBCI *DVBCI2;
#endif

public:
	enum
	{
		controllerNone,
		controllerScan,
		controllerService
	};
	

protected:
	int controllertype;
	eDVBController *controller;

private:
	void tunedIn(eTransponder*, int);
	eDVBState state;
	int mID;
public:
	
	void setMode(int mode);

	const eDVBState &getState() const { return state; }
	void setState(const eDVBState &newstate) { /*emit*/ stateChanged(state=newstate); }
	void event(const eDVBEvent &event);

	Signal1<void, const eDVBState&> stateChanged;
	Signal1<void, const eDVBEvent&> eventOccured;

		// -> noch woanders hin
	Signal0<void> timeUpdated;
	
public:
	eString getVersion();
	eDVB();
	~eDVB();
	static eDVB *getInstance()
	{
		return instance;
	}

	PMT *getPMT();
	EIT *getEIT();
	SDT *getSDT();

	/**
	 * \brief Configures the network.
	 *
	 * Configures the network according to the configuration stored in the registry.
	 */
	void configureNetwork();

#ifndef DISABLE_FILE
			// recording
		/// starts a new recording
	void recBegin(const char *filename, eServiceReferenceDVB service);
		/// pauses a recording
	void recPause();
		/// resumes a recording
	void recResume();
		/// closes a recording
	void recEnd();
#endif //DISABLE_FILE
	
	int time_difference;
	
	/* container for settings */
	eDVBSettings *settings;

	eDVBServiceController *getServiceAPI();
	eDVBScanController *getScanAPI();

	Signal1<void, bool> scrambled;
	Signal0<void> serviceListChanged;
	Signal0<void> bouquetListChanged;
	Signal1<void, const eServiceReferenceDVB &> leaveService;
	Signal1<void, const eServiceReferenceDVB &> enterService;
	Signal1<void, eTransponder*> leaveTransponder;
	Signal1<void, eTransponder*> enterTransponder;
	Signal2<void, eTransponder*, int> switchedTransponder;
	Signal2<void, const eServiceReferenceDVB &, int> switchedService;
	Signal2<void, EIT*, int> gotEIT;
	Signal1<void, SDT*> gotSDT;
	Signal1<void, PMT*> gotPMT;
};

#endif
