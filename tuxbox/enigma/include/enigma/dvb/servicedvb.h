#ifndef __core_dvb_servicedvb_h
#define __core_dvb_servicedvb_h

#include <core/dvb/service.h>
#include <core/dvb/servicecache.h>
#include <core/base/thread.h>
#include <core/base/buffer.h>
#include <core/base/message.h>

class eServiceHandlerDVB;

class eDVRPlayerThread: public eThread, public eMainloop, public Object
{
	eServiceHandlerDVB *handler;
	eIOBuffer buffer;
	enum
	{
		stateInit, stateError, stateBuffering, stateBufferFull, statePlaying, statePause, stateFileEnd
	};
	int state;
	int dvrfd;
	int sourcefd;
	int speed;
	eSocketNotifier *inputsn, *outputsn;
	void readMore(int what);
	void outputReady(int what);
	int maxBufferSize;
	eLock lock;
	
	void dvrFlush();
public:
	struct eDVRPlayerThreadMessage
	{
		enum
		{
			start, exit,
			skip,
			setSpeed, // 0..
			seek,	// 0..65536
			seekreal
		};
		int type;
		int parm;
		eDVRPlayerThreadMessage() { }
		eDVRPlayerThreadMessage(int type): type(type) { }
		eDVRPlayerThreadMessage(int type, int parm): type(type), parm(parm) { }
	};
	eFixedMessagePump<eDVRPlayerThreadMessage> messages;
	
	void gotMessage(const eDVRPlayerThreadMessage &message);
	
	eDVRPlayerThread(const char *filename, eServiceHandlerDVB *handler);
	~eDVRPlayerThread();
	
	void thread();
};


class eServiceHandlerDVB: public eServiceHandler
{
	void addFile(void *node, const eString &filename);
	friend class eDVRPlayerThread;

	struct eDVRPlayerThreadMessage
	{
		enum
		{
			done,
			status
		};
		int type;
		int parm;
		eDVRPlayerThreadMessage() { }
		eDVRPlayerThreadMessage(int type): type(type) { }
		eDVRPlayerThreadMessage(int type, int status): type(type), parm(parm) { }
	};
	eFixedMessagePump<eDVRPlayerThreadMessage> messages;
	eDVRPlayerThread *decoder;
	
	void gotMessage(const eDVRPlayerThreadMessage &message);

	void scrambledStatusChanged(bool);
	void switchedService(const eServiceReferenceDVB &, int);
	void gotEIT(EIT *eit, int);
	void gotSDT(SDT *sdt);
	void gotPMT(PMT *pmt);
	void leaveService(const eServiceReferenceDVB &);
	void aspectRatioChanged(int ratio);
	int flags, state, aspect, error;
	
	eServiceCache<eServiceHandlerDVB> cache;
public:
	int getID() const;
	eServiceHandlerDVB();
	~eServiceHandlerDVB();
	eService *lookupService(const eServiceReference &service);

	int play(const eServiceReference &service);

		// record	
	int serviceCommand(const eServiceCommand &cmd);

		// for DVB audio channels:
	PMT *getPMT();
	void setPID(const PMTEntry *);
	
		// for DVB nvod channels:
	SDT *getSDT();

		// for DVB events, nvod, audio....
	EIT *getEIT();
	
	int getFlags();
	int getAspectRatio();
	int getState();
	int getErrorInfo();

	int stop();

	void loadNode(eServiceCache<eServiceHandlerDVB>::eNode &node, const eServiceReference &ref);
	eService *createService(const eServiceReference &node);

	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);
};

#endif
