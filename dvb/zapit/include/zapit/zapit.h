#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#include <string>
#include <map>

/* NAPI */
#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#include <ost/ca.h>
#include <ost/audio.h>
#include <dbox/avia_vbi.h>

#include "bouquets.h"
#include "getservices.h"
#include "sdt.h"
#include "zapitclient.h"

#include "config.h"

#ifdef OLD_TUNER_API
#define FRONT_DEV "/dev/ost/qpskfe0"
#else
#define FRONT_DEV "/dev/ost/frontend0"
#endif
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV   "/dev/ost/sec0"
#define VIDEO_DEV "/dev/ost/video0"
#define AUDIO_DEV "/dev/ost/audio0"

#define CAT_SIZE	1024
#define PMT_SIZE	1024

#ifdef DVBS
#define USE_EXTERNAL_CAMD
#endif

int dvb_device;

#define SA struct sockaddr
#define SAI struct sockaddr_in

int listenfd, connfd;
socklen_t clilen;
SAI cliaddr, servaddr;
int offset=0;
int caid = 0;
int caver = 0;

struct rmsg {
  		unsigned char version;
  		unsigned char cmd;
  		unsigned char param;
  		unsigned short param2;
  		char param3[30];

} rmsg;

typedef struct decode_struct{
	uint onid;
	uint tsid;
	uint ecmpid;
	pids *parse_pmt_pids;
	bool do_search_emmpids;
	bool do_cam_reset;
} decode_vals;

int LoadServices();
void nit();
int pat(uint oonid,std::map<uint,channel> *cmap);
int sdt(uint osid,bool scan_mode);
int tune(uint tsid);
void *start_scanthread(void *);

#ifndef USE_EXTERNAL_CAMD
int get_caid();
int get_caver();
#endif

/**************************************************************/
/*                                                            */
/*  functions for new command handling via CZapitClient       */
/*                                                            */
/*  these functions should be encapsulated in a class CZapit  */
/*                                                            */
/**************************************************************/

void addChannelToBouquet(unsigned int bouquet, unsigned int onid_sid);
void removeChannelFromBouquet(unsigned int bouquet, unsigned int onid_sid);
void sendBouquets(bool emptyBouquetsToo);
void internalSendChannels( ChannelList* channels);
void sendBouquetChannels(unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels(CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
void startPlayBack();
void stopPlayBack();
void zapTo(unsigned int channel);
void zapTo(unsigned int bouquet, unsigned int channel);

