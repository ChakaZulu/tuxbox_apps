#ifndef __lib_dvb_servicedvb_h
#define __lib_dvb_servicedvb_h

#include <lib/dvb/service.h>
#include <lib/dvb/servicecache.h>
#include <lib/base/thread.h>
#include <lib/base/buffer.h>
#include <lib/base/message.h>
#include <lib/dvb/edvb.h>


class eServiceHandlerDVB;

#ifndef DISABLE_FILE

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
	int audiotracks;    // num of audiotracks in file

	int livemode; 		// used in timeshift where end-of-file is only temporary
	eTimer liveupdatetimer;
	void updatePosition();

	int filelength, // in 1880 packets
	    buffersize; // bytes in enigma buffer...
	off64_t position;

	off64_t slicesize;
	eString filename;
	eSocketNotifier *inputsn, *outputsn;
	void readMore(int what);
	void outputReady(int what);
	int maxBufferSize;
	int seekbusy, seeking;

	void dvrFlush();
	int openFile(int slice=0);
	void seekTo(off64_t offset);
	int getBufferSize();
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
			seekmode,
			updateAudioTracks,
		};
		int type;
		int parm;
		eDVRPlayerThreadMessage() { }
		eDVRPlayerThreadMessage(int type): type(type) { }
		eDVRPlayerThreadMessage(int type, int parm): type(type), parm(parm) { }
	};
	eFixedMessagePump<eDVRPlayerThreadMessage> messages;
	
	void gotMessage(const eDVRPlayerThreadMessage &message);
	
	eDVRPlayerThread(const char *filename, eServiceHandlerDVB *handler, int livemode);
	~eDVRPlayerThread();

	int getPosition(int);
	int getLength(int);

	void thread();
};

#endif //DISABLE_FILE

class eServiceHandlerDVB: public eServiceHandler
{
#ifndef DISABLE_FILE
	friend class eDVRPlayerThread;
	int recording;

	struct eDVRPlayerThreadMessage
	{
		enum
		{
			done,
			liveeof,
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
	eString current_filename;

			// (u.a.) timeshift:
	void startPlayback(const eString &file, int livemode);
	void stopPlayback(int waslivemode=0);

	void gotMessage(const eDVRPlayerThreadMessage &message);
	void handleDVBEvent( const eDVBEvent& );
#endif //DISABLE_FILE

	void scrambledStatusChanged(bool);
	void switchedService(const eServiceReferenceDVB &, int);
	void gotEIT(EIT *eit, int);
	void gotSDT(SDT *sdt);
	void gotPMT(PMT *pmt);
	void leaveService(const eServiceReferenceDVB &);
	void aspectRatioChanged(int ratio);
	int state, aspect, error;
	
	int pcrpid;

	eServiceCache<eServiceHandlerDVB> cache;
public:
	int getID() const;
	eServiceHandlerDVB();
	~eServiceHandlerDVB();
	eService *lookupService(const eServiceReference &service);

	int play(const eServiceReference &service, int workaround=0 );

		// record	
	int serviceCommand(const eServiceCommand &cmd);

		// for DVB audio channels:
	PMT *getPMT();
	void setPID(const PMTEntry *);
	
		// for DVB nvod channels:
	SDT *getSDT();

		// for DVB events, nvod, audio....
	EIT *getEIT();
	
	int getAspectRatio();
	int getState();
	int getErrorInfo();

	int stop( int workaround = 0 );

	void loadNode(eServiceCache<eServiceHandlerDVB>::eNode &node, const eServiceReference &ref);
	eService *createService(const eServiceReference &node);
#ifndef DISABLE_FILE
	void addFile(void *node, const eString &filename);
#endif

	void enterDirectory(const eServiceReference &dir, Signal1<void,const eServiceReference&> &callback);
	void leaveDirectory(const eServiceReference &dir);

	eService *addRef(const eServiceReference &service);
	void removeRef(const eServiceReference &service);

	int getPosition(int what);
};

#endif
