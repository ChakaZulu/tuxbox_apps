#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <string>
#include <map>
#include <sys/un.h>

/* NAPI */
#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#include <ost/ca.h>
#include <ost/audio.h>

#define OLD_TUNER_API

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "getservices.h"
#include "sdt.h"

/*Thread stuff */
#include <pthread.h>

#ifdef OLD_TUNER_API
#define FRONT_DEV "/dev/ost/qpskfe0"
#else
#define FRONT_DEV "/dev/ost/frontend0"
#endif
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV   "/dev/ost/sec0"
#define VIDEO_DEV   "/dev/ost/video0"
#define AUDIO_DEV "/dev/ost/audio0"

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
int LoadBouquets();
int get_caid();
int sdt(uint osid,bool scan_mode);
int pat(uint oonid,std::map<uint,channel> *cmap);
void nit();
int tune(uint tsid);
void *start_scanthread(void *);
int get_caver();
