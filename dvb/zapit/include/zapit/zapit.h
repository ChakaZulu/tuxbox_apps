#ifndef __zapit_h__
#define __zapit_h__

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
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
#include "ca-ids.h"
#include "nit.h"
#include "pat.h"
#include "sdt.h"
#include "tune.h"
#include "zapitclient.h"
#include "eventserver.h"

#include "config.h"

#define FRONT_DEV "/dev/ost/frontend0"
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV   "/dev/ost/sec0"
#define VIDEO_DEV "/dev/ost/video0"
#define AUDIO_DEV "/dev/ost/audio0"

#define CAT_SIZE	1024
#define PMT_SIZE	1024

#ifdef DVBS
#define USE_EXTERNAL_CAMD
#endif

typedef struct decode_struct{
	uint onid;
	uint tsid;
	uint ecmpid;
	pids *parse_pmt_pids;
	bool do_search_emmpids;
	bool do_cam_reset;
} decode_vals;

int LoadServices();
void *start_scanthread(void *);

#ifndef USE_EXTERNAL_CAMD
int writecam(uint8_t *data, uint8_t len);
int cam_reset();
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
unsigned int zapTo(unsigned int channel);
unsigned int zapTo(unsigned int bouquet, unsigned int channel);
unsigned int zapTo_ServiceID(unsigned int serviceID, bool isSubService );
void sendAPIDs();

#endif /* __zapit_h__ */
