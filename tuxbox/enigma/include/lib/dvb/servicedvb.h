#ifndef __lib_dvb_servicedvb_h
#define __lib_dvb_servicedvb_h

#include <lib/dvb/service.h>
#include <lib/dvb/servicecache.h>
#include <lib/base/thread.h>
#include <lib/base/buffer.h>
#include <lib/base/message.h>
#include <lib/dvb/edvb.h>


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
	int slice;

	int filelength; // in 1880 packets
	int position;
	eLock poslock;

	off64_t slicesize;
	eString filename;
	eSocketNotifier *inputsn, *outputsn;
	void readMore(int what);
	void outputReady(int what);
	int maxBufferSize;
	int seekbusy, seeking;
	eLock lock;
	
	void dvrFlush();
	int openFile(int slice=0);
public:
	struct eDVRPlayerThreadMessage
	{
		enum
		{
			start, exit,
			skip,
			setSpeed, // 0..
			seek,	// 0..65536
			seekreal,
			seekmode
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

	int getPosition(int);
	int getLength(int);
	
	void thread();
};


class eServiceHandlerDVB: public eServiceHandler
{
	friend class eDVRPlayerThread;
	int recording;

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
	void handleDVBEvent( const eDVBEvent& );
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
	void addFile(void *node, const eString &filename);

	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);

	int getPosition(int what);
};

#endif
