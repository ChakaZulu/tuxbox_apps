#include <lib/dvb/dvbci.h>
#include <lib/system/init.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/si.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

//external defines (because missing dreambox header-files in cvs)
#define CI_RESET					0
#define CI_SOFTRESET			1
#define CI_GET_BUFFERSIZE	2
#define CI_GET_STATUS			3
#define CI_ACTIVATE				4
#define CI_DEACTIVATE			5
#define CI_TS_ACTIVATE		6
#define CI_TS_DEACTIVATE	7

#define MAX_SESSIONS			32
#define STATE_FREE				0
#define STATE_OPEN				1

struct session_struct
{
	unsigned int tc_id;
	unsigned long service_class;
	unsigned int state;
	unsigned int internal_state;
};

struct session_struct sessions[32];


eDVBCI::eDVBCI():pollTimer(this),messages(this, 1)
{
	state=stateInit;

	eDebug("[DVBCI] start");

	fd=::open("/dev/ci",O_RDWR|O_NONBLOCK);
	if (fd<0)
	{
		eDebug("[DVBCI] error opening /dev/ci");
		state=stateError;
	}
	
	ci=new eSocketNotifier(this, fd, eSocketNotifier::Read, 0);
	CONNECT(ci->activated, eDVBCI::dataAvailable);
	CONNECT(pollTimer.timeout,eDVBCI::poll);
	CONNECT(messages.recv_msg, eDVBCI::gotMessage);
	
	memset(appName,0,sizeof(appName));

	if (state != stateError)
		run();
}

void eDVBCI::thread()
{
	messages.start();
	exec();
}

eDVBCI::~eDVBCI()
{
	kill(); // wait for thread exit.

	delete ci;
	if (fd >= 0)
		close(fd);
}

void eDVBCI::gotMessage(const eDVBCIMessage &message)
{
	switch (message.type)
	{
	case eDVBCIMessage::start:
		if (state == stateInit)
		{
			ci->start();
			ci_state=0;
		}
		break;
	case eDVBCIMessage::reset:
		eDebug("[DVBCI] got reset message..");
		if(!ci_state)
			ci_progress("no module");	
		ci_state=0;
		clearCAIDs();
		break;
	case eDVBCIMessage::init:
		eDebug("[DVBCI] got init message..");
		if(ci_state)
			sendCAPMT();
		else
			ci_progress("no module");	
		break;
	case eDVBCIMessage::flush:
		eDebug("[DVBCI] got flush message..");
		createCAPMT(0,0);		
		break;
	case eDVBCIMessage::addDescr:
		eDebug("[DVBCI] got addDescr message..");
		createCAPMT(1,message.data);		
		break;
	case eDVBCIMessage::es:
		eDebug("[DVBCI] got es message..");
		createCAPMT(2,0);		
		break;
	case eDVBCIMessage::addVideo:
		eDebug("[DVBCI] got addVideo message..");
		CAPMT[CAPMTpos++]=0x2;
		CAPMT[CAPMTpos++]=(unsigned char)(message.pid>>8);
		CAPMT[CAPMTpos++]=(unsigned char)message.pid;
		CAPMTdescrpos=CAPMTpos;
		CAPMTdescrlen=0;
		CAPMT[CAPMTpos++]=0x0;
		CAPMT[CAPMTpos++]=0x0;
		CAPMTlen+=5;
		break;
	case eDVBCIMessage::addAudio:
		eDebug("[DVBCI] got addAudio message..");
		CAPMT[CAPMTdescrpos]=(CAPMTdescrlen >> 8);
		CAPMT[CAPMTdescrpos+1]=(CAPMTdescrlen & 0xff);
		CAPMT[CAPMTpos++]=0x4;
		CAPMT[CAPMTpos++]=(unsigned char)(message.pid>>8);
		CAPMT[CAPMTpos++]=(unsigned char)message.pid;
		CAPMTdescrpos=CAPMTpos;
		CAPMTdescrlen=0;
		CAPMT[CAPMTpos++]=0x0;
		CAPMT[CAPMTpos++]=0x0;
		CAPMTlen+=5;
		break;
	case eDVBCIMessage::go:
		eDebug("[DVBCI] got go message..");
		CAPMT[7]=CAPMTlen-8;
		CAPMT[CAPMTdescrpos]=(CAPMTdescrlen >> 8);
		CAPMT[CAPMTdescrpos+1]=(CAPMTdescrlen & 0xff);
		if(ci_state)
			sendCAPMT();
		break;
	case eDVBCIMessage::exit:
		eDebug("[DVBCI] got exit message..");
		quit();
		break;
	}
}

void eDVBCI::createCAPMT(int type,unsigned char *data)
{
	switch(type)
	{
		case 0:				//flush
			memcpy(CAPMT,"\x90\x2\x0\x3\x9f\x80\x32\x0\x3\x0\x0\x0",12);
			CAPMTlen=14;
			CAPMTpos=14;
			CAPMT[7]=0;
			break;
		case 1:				//descriptor
			eDebug("descr:%x",data[0]);
			CAPMT[CAPMTpos++]=0x1;
			memcpy(CAPMT+CAPMTpos,data,data[1]+2);
			CAPMTpos+=(data[1]+2);
			CAPMTlen+=(data[1]+3);
			CAPMTdescrlen+=(data[1]+3);
			break;
		case 2:				//mode es
			CAPMT[12]=((CAPMTlen-14)<<8);
			CAPMT[13]=((CAPMTlen-14) & 0xff);
			break;
	}		
}

void eDVBCI::sendCAPMT()
{
	int i;
	printf("CA-PMT:");
	for(i=0;i<CAPMTlen;i++)
		printf("%02x ",CAPMT[i]);
	printf("\n");	
	
	if(CAPMTlen>0)
		sendTPDU(0xA0,CAPMTlen,1,CAPMT);

	ci_progress(appName);
	
}

void eDVBCI::clearCAIDs()
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if(!sapi)
		return;
		
	std::list<int>& availCA = sapi->availableCASystems;
	
	availCA.clear();
}

void eDVBCI::addCAID(int caid)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if(!sapi)
		return;
		
	std::list<int>& availCA = sapi->availableCASystems;
	
	availCA.push_back(caid);

}

void eDVBCI::sendTPDU(unsigned char tpdu_tag,unsigned int len,unsigned char tc_id,unsigned char *data)
{
	unsigned char buffer[len+5];
	
	buffer[0]=tc_id;
	buffer[1]=0;
	buffer[2]=tpdu_tag;
	buffer[3]=len+1;
	buffer[4]=tc_id;
	memcpy(buffer+5,data,len);
	
	write(fd,buffer,len+5);
}

void eDVBCI::help_manager(unsigned int session)
{
  switch(sessions[session].internal_state)
  {
    case 0:
      {
        unsigned char buffer[12];
        eDebug("[DVBCI] [HELP MANAGER] up to now nothing happens -> profile_enq");

        memcpy(buffer,"\x90\x2\x0\x1\x9f\x80\x10\x0",8);
        sendTPDU(0xA0,8,sessions[session].tc_id,buffer);

        sessions[session].internal_state=1;
        break;
      }
    case 1:
      {
        unsigned char buffer[12];

 				eDebug("[DVBCI] [HELP MANAGER] profile_change");

        memcpy(buffer,"\x90\x2\x0\x1\x9f\x80\x12\x0",8);
        sendTPDU(0xA0,8,sessions[session].tc_id,buffer);

        sessions[session].internal_state=2;
        break;
      }
    case 2:
      {
        unsigned char buffer[30];

        eDebug("[DVBCI] [HELP MANAGER] profile_reply");
        //was wir alles koennen :)
        memcpy(buffer,"\x90\x2\x0\x1\x9f\x80\x11",7);

        buffer[7]=0x14;

        buffer[8]=0x00;
        buffer[9]=0x01;
        buffer[10]=0x00;
        buffer[11]=0x41;
        buffer[12]=0x00;
        buffer[13]=0x00;
        buffer[14]=0x00;
        buffer[15]=0x41;
        buffer[16]=0x00;
        buffer[17]=0x03;
        buffer[18]=0x41;
        buffer[19]=0x00;
        buffer[20]=0x24;
        buffer[21]=0x00;
        buffer[22]=0x41;
        buffer[23]=0x00;
        buffer[24]=0x40;
        buffer[25]=0x00;

        sendTPDU(0xA0,26,sessions[session].tc_id,buffer);
        sessions[session].internal_state=3;
        break;
      }
    default:
      //printf("[HELP MANAGER] undefined state\n");
      break;
  }
}

void eDVBCI::app_manager(unsigned int session)
{
  switch(sessions[session].internal_state)
  {
    case 0:
      {
        unsigned char buffer[12];
        eDebug("[DVBCI] [APPLICATION MANAGER] up to now nothing happens -> app_info_enq");
        memcpy(buffer,"\x90\x2\x0\x2\x9f\x80\x20\x0",8);
        sendTPDU(0xA0,8,sessions[session].tc_id,buffer);

        sessions[session].internal_state=1;
        break;
      }
    case 1:
      break;
    default:
      break;
  }


}


void eDVBCI::ca_manager(unsigned int session)
{
  switch(sessions[session].internal_state)
  {
    case 0:
      {
        unsigned char buffer[12];
				clearCAIDs();
        eDebug("[DVBCI] [CA MANAGER] up to now nothing happens -> ca_info_enq");

        memcpy(buffer,"\x90\x2\x0\x3\x9f\x80\x30\x0",9);
        sendTPDU(0xA0,8,sessions[session].tc_id,buffer);

        sessions[session].internal_state=1;
        break;
      }
    case 1:
      {
        unsigned char buffer[100];
        eDebug("[DVBCI] [CA MANAGER] send ca_pmt\n");
				
				::ioctl(fd,CI_TS_ACTIVATE);	

        memcpy(buffer,"\x90\x2\x0\x3\x9f\x80\x32",7);

        buffer[7]=23;       //laenge

        buffer[8]=0x03;     //one and only
        buffer[9]=0x0F;     //prg nr
        buffer[10]=0xAD;    //prg nr
        buffer[11]=0xE5;    //current-next/version
        buffer[12]=0x00;    //info len
        buffer[13]=0x07;    //info len
        buffer[14]=0x01;    //ca-pmt_cmd_id
        buffer[15]=0x09;    //descriptor
        buffer[16]=0x04;
        buffer[17]=0x06;
        buffer[18]=0x02;
        buffer[19]=0xe5;
        buffer[20]=0x05;

        buffer[21]=0x02;    //stream type
        buffer[22]=0xe0;    //pid
        buffer[23]=0xa4;
        buffer[24]=0x00;    //es-info length
        buffer[25]=0x00;

        buffer[26]=0x04;    //stream type
        buffer[27]=0xe0;    //pid
        buffer[28]=0x60;
        buffer[29]=0x00;    //es-info length
        buffer[30]=0x00;
        //sendTPDU(0xA0,31,sessions[session].tc_id,buffer);
				sendCAPMT();
        sessions[session].internal_state=2;

        break;
      }
    default:
      break;
  }
}

void eDVBCI::handle_session(unsigned char *data,int len)
{
	if(data[4]==0x9f && data[5]==0x80 && data[6]==0x11)
		help_manager(1);
		
	if(data[4]==0x9f && data[5]==0x80 && data[6]==0x10)
	{
		help_manager(1);
		ci_progress("help-manager init");

	}	
	if(data[4]==0x9f && data[5]==0x80 && data[6]==0x21)
	{
		eDebug("[DVBCI] APP-INFO");
		memcpy(appName,data+14,data[13]);
		appName[data[13]]=0x0;
		ci_progress("application manager-init");
	}
	
	if(data[4]==0x9f && data[5]==0x80 && data[6]==0x31)
	{
		int i;
		eDebug("[DVBCI] CA-INFO");
		
		if(data[7]>(len+8))
			eDebug("[DVBCI] [CA MANAGER] error in ca-info");

		ci_progress("ca-manager init");
			
		for(i=8;i<data[7]+8;i+=2)
		{
			eDebug("[DVBCI] [CA MANAGER] add CAID: %04x",data[i]<<8|data[i+1]);
			addCAID(data[i]<<8|data[i+1]);
		}	
		ca_manager(3);

	}
				
}

int eDVBCI::service_available(unsigned long service_class)
{
	switch(service_class)
	{
		case 0x010041:
		case 0x020041:
		case 0x030041:
		case 0x034100:
		case 0x400041:
		case 0x240041:
			return 1;
		default:
			return 0;
	}
}				

void eDVBCI::handle_spdu(unsigned int tpdu_tc_id,unsigned char *data,int len)
{
	unsigned char buffer[40];
	
	switch(data[0])
	{
		case 0x90:
			handle_session(data,len);
			break;
		case 0x91:
			if(service_available(*(unsigned long*)(data+2)))
			{
				int i;
				for(i=1;i<MAX_SESSIONS;i++)
					if(sessions[i].state==STATE_FREE)
						break;
				if(i==MAX_SESSIONS)
				{
					eDebug("[DVBCI] no free sessions left");
					memcpy(buffer,"\x92\x7\xf0\x0\x0\x0\x0\x0\x0",9);
					memcpy(buffer+3,data+2,4);
					sendTPDU(0xA0,9,tpdu_tc_id,buffer);
					return;
				}
				sessions[i].state=STATE_OPEN;
				sessions[i].service_class=*(unsigned long*)(data+2);
				sessions[i].tc_id=tpdu_tc_id;
				sessions[i].internal_state=0;
				
				memcpy(buffer,"\x92\x7\x0",3);
				memcpy(buffer+3,data+2,4);
				buffer[7]=i>>8;
				buffer[8]=i& 0xff;
				sendTPDU(0xA0,9,tpdu_tc_id,buffer);
				
				if(sessions[i].service_class==0x10041)
					help_manager(i);
				if(sessions[i].service_class==0x20041)
					app_manager(i);
				if(sessions[i].service_class==0x30041)
					ca_manager(i);
				if(sessions[i].service_class==0x34100)
					ca_manager(i);
			}
			else
			{
				eDebug("[DVBCI] unknown serviceclass (%x) requested",*(unsigned long*)(data+2));
				memcpy(buffer,"\x92\x7\xf0",3);
				memcpy(buffer+3,data+2,4);
				sendTPDU(0xA0,9,tpdu_tc_id,buffer);
			}
			break;
		default:
			eDebug("[DVBCI] unknown SPDU-TAG:%x",data[0]);		
	}
}

void eDVBCI::receiveTPDU(unsigned char tpdu_tag,unsigned int tpdu_len,unsigned char tpdu_tc_id,unsigned char *data)
{
	//unsigned char buffer[40]; //temporary	
	
	switch(tpdu_tag)
	{
		case 0x80:
			if(data[0]==0x80)
				sendTPDU(0x81,0,tpdu_tc_id,0);
			break;
		case 0x83:
			eDebug("[DVBCI] T_C_ID %d wurde erstellt",tpdu_tc_id);	
			break;
		case 0xA0:
			if(tpdu_len)
			{
				if(data[0] >= 0x90 && data[0] <= 0x96)
				{
					handle_spdu(tpdu_tc_id,data,tpdu_len);
				}
				else
				{
					eDebug("[DVBCI] unknown spdu-tag:%x",data[0]);	
				}
			}
			break;
	}		
}

void eDVBCI::incoming(unsigned char *buffer,int len)
{
	int tc_id;
	int m_l;
	int tpdu_tag;
	int tpdu_len;
	int tpdu_tc_id;
	int x=0;
	
	tc_id=buffer[x++];
	m_l=buffer[x++];
	while(x<len)
	{
		tpdu_tag=buffer[x++];
		tpdu_len=buffer[x++]-1;
		tpdu_tc_id=buffer[x++];
		
		if(tpdu_len==0x81)				//find the mistake
		{
			tpdu_len=buffer[x++]-1;
			tpdu_tc_id=buffer[x++];
		}	
		
		receiveTPDU(tpdu_tag,tpdu_len,tpdu_tc_id,buffer+x);
		x+=tpdu_len;
	}	
}

void eDVBCI::dataAvailable(int what)
{
	int present;
	unsigned char buffer[256];
	int size;

	pollTimer.stop();

	::ioctl(fd,CI_GET_STATUS,&present);	

	if(present!=1)						//CI removed
	{	
		eDebug("[DVBCI] module removed");	
		memset(appName,0,sizeof(appName));
		ci_progress("no module");
		
		::read(fd,&buffer,0);	
		ci_state=0;
		clearCAIDs();
		return;
	}		
	
	if(ci_state==0)						//CI plugged
	{
		int i;
		eDebug("[DVBCI] module inserted");	
		ci_progress("module found");

		for(i=0;i<MAX_SESSIONS;i++)
			sessions[i].state=STATE_FREE;

		::read(fd,&buffer,0);	
	
		::ioctl(fd,CI_ACTIVATE);	
		if(::ioctl(fd,CI_RESET)!=0)
		{
			ci_state=0;
			clearCAIDs();
			return;
		}		
	
		ci_state=1;
	}					
		
	size=::read(fd,&buffer,sizeof(buffer));
	//eDebug("READ:%d",size);	

	if(size>0)
		incoming(buffer,size);			
	
	if(ci_state==1)
	{
		sendTPDU(0x82,0,1,0);	
		ci_state=2;
	}
	pollTimer.start(200);
}

void eDVBCI::poll()
{
	int present;

	::ioctl(fd,CI_GET_STATUS,&present);	

	if(present)						//CI removed
		sendTPDU(0xA0,0,1,0);
}
