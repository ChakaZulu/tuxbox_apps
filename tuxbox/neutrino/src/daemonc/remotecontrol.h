#ifndef __remotecontrol__
#define __remotecontrol__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include <string>

#include "pthread.h"
#include "semaphore.h"
#include <sys/wait.h>
#include <signal.h>
#include "../../../zapit/getservices.h"

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

struct st_rmsg
{
	unsigned char version;
 	unsigned char cmd;
	unsigned char param;
	unsigned short param2;
	char param3[30];
};

struct apid_info
{
    char    name[50];
    int     ctag;
    bool    is_ac3;
};

struct st_audio_info
{
    char        name[100];
    ushort      count_apids;
    apid_info   apids[max_num_apids];
    int         selected;
};

struct nvod_info
{
    time_t          startzeit;
    unsigned int    onid_sid;
    unsigned short  tsid;
};


struct st_nvod_info
{
    char        name[100];
    ushort      count_nvods;
    nvod_info   nvods[10];
    int         selected;
};



class CRemoteControl
{
        st_rmsg		remotemsg;
        st_audio_info	audio_chans_int;
        st_nvod_info    nvods_int;
        unsigned int    ecm_pid;

		void send();
		bool zapit_mode;

        pthread_t       thrSender;
        pthread_cond_t  send_cond;
        pthread_mutex_t send_mutex;

        static void * RemoteControlThread (void *arg);

	public:
        st_audio_info   audio_chans;
        st_nvod_info    nvods;

		CRemoteControl();
        void zapTo_onid_sid( unsigned int onid_sid );
		void zapTo( string chnlname );
        void queryAPIDs();
        void setAPID(int APID);
        void setNVOD(int NVOD);
		void shutdown();
		void setZapper (bool zapper);
		bool getZapper(){return zapit_mode;}
		void radioMode();
		void tvMode();

        void CopyAPIDs();
        void CopyNVODs();
        unsigned int GetECMPID();
};


#endif



