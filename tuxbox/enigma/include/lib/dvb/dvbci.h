#ifndef __core_dvb_ci_h
#define __core_dvb_ci_h

#include <lib/dvb/service.h>
//#include <lib/base/buffer.h>

#include <lib/base/ebase.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/system/elock.h>

class eDVBCI: private eThread, public eMainloop, public Object
{
	enum
	{
		stateInit, stateError, statePlaying, statePause
	};
	int state;
	int fd;
	eSocketNotifier *ci;

	int ci_state;
	int buffersize;	
		
	eTimer pollTimer;
	eLock lock;

	int tempPMTentrys;

	char appName[256];
	unsigned short caids[256];
	unsigned int caidcount;
	
	unsigned char ml_buffer[1024];
	int ml_bufferlen;
	int ml_buffersize;
			
	void clearCAIDs();
	void addCAID(int caid);	
	void pushCAIDs();	
	void PMTflush(int program);
	void PMTaddPID(int pid,int streamtype);
	void PMTaddDescriptor(unsigned char *data);
	void newService();
	void create_sessionobject(unsigned char *tag,unsigned char *data,unsigned int len,int session);

	void sendData(unsigned char tc_id,unsigned char *data,unsigned int len);	
	void sendTPDU(unsigned char tpdu_tag,unsigned int len,unsigned char tc_id,unsigned char *data);
	void help_manager(unsigned int session);
	void app_manager(unsigned int session);
	void ca_manager(unsigned int session);
	
	void handle_session(unsigned char *data,int len);
	int service_available(unsigned long service_class);
	void handle_spdu(unsigned int tpdu_tc_id,unsigned char *data,int len);	
	void receiveTPDU(unsigned char tpdu_tag,unsigned int len,unsigned char tpdu_tc_id,unsigned char *data);
	void incoming(unsigned char *buffer,int len);
	void dataAvailable(int what);
	void poll();
	void updateCIinfo(unsigned char *buffer);

	void mmi_begin();
	void mmi_end();
	void mmi_answ(unsigned char *answ,int len);
	void mmi_menuansw(int);

					
public:
	struct eDVBCIMessage
	{
		enum
		{
			start,
			reset, 
			init,
			exit,
			flush,
			addDescr,
			addVideo,
			addAudio,
			es,
			go,
			PMTflush,
			PMTaddPID,
			PMTaddDescriptor,
			mmi_begin,
			mmi_end,
			mmi_answ,
			mmi_menuansw,
			getcaids,
		};
		int type;
		unsigned char *data;
		int pid;
		int streamtype;

		eDVBCIMessage() { }
		eDVBCIMessage(int type): type(type) { }
		eDVBCIMessage(int type, unsigned char *data): type(type), data(data) { }
		eDVBCIMessage(int type, int pid): type(type),pid(pid) { }
		eDVBCIMessage(int type, int pid, int streamtype): type(type),pid(pid),streamtype(streamtype) { }

	};
	eFixedMessagePump<eDVBCIMessage> messages;
	
	void gotMessage(const eDVBCIMessage &message);

	eDVBCI();
	~eDVBCI();
	
	void thread();
	Signal1<void, const char*> ci_progress;
	Signal1<void, const char*> ci_mmi_progress;

};

//rewrite starts here
struct _lpduQueueElem
{
	unsigned char lpduLen;
	unsigned char lpdu[256];		//fixed buffer-size (pcmcia)
	_lpduQueueElem *nextElem;
};

typedef struct _lpduQueueElem * ptrlpduQueueElem;	

ptrlpduQueueElem AllocLpduQueueElem(unsigned char t_c_id);	
void SendLPDU(unsigned char lpdu,unsigned char length);
void LinkSendData(unsigned char t_c_id, unsigned char *toSend, long numBytes);
void lpduQueueElemSetMore(ptrlpduQueueElem curElem, int more);
#endif
