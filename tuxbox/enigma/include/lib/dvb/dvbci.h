#ifndef __lib_dvb_ci_h
#define __lib_dvb_ci_h

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

	unsigned char CAPMT[256];
	int CAPMTlen;
	int CAPMTpos;
	int CAPMTstate;
	int CAPMTdescrpos;
	int CAPMTdescrlen;
		
	void createCAPMT(int type,unsigned char *data);
	void sendCAPMT();
	void clearCAIDs();
	void addCAID(int caid);	
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
		};
		int type;
		union
		{
			unsigned char *data;
			int pid;
		};	
		eDVBCIMessage() { }
		eDVBCIMessage(int type): type(type) { }
		eDVBCIMessage(int type,unsigned char *data): type(type),data(data) { }
		eDVBCIMessage(int type,int pid): type(type),pid(pid) { }

	};
	eFixedMessagePump<eDVBCIMessage> messages;
	
	void gotMessage(const eDVBCIMessage &message);

	eDVBCI();
	~eDVBCI();
	
	void thread();

};
#endif
