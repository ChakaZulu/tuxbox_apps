/*
 * $Id: zapit.h,v 1.45 2002/04/04 21:26:08 obi Exp $
 */

#ifndef __zapit_h__
#define __zapit_h__

#include <dbox/avia_vbi.h>
#include <fcntl.h>
#include <ost/audio.h>
#include <ost/video.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bouquets.h"
#include "cam.h"
#include "cat.h"
#include "getservices.h"
#include "pat.h"
#include "pmt.h"
#include "tune.h"
#include "zapitclient.h"

#define AUDIO_DEV "/dev/ost/audio0"
#define FRONT_DEV "/dev/ost/frontend0"
#define VIDEO_DEV "/dev/ost/video0"

typedef struct decode_struct {
	uint16_t onid;
	uint16_t tsid;
	dvb_pid_t ecmpid;
	pids *parse_pmt_pids;
	bool do_search_emmpids;
	bool do_cam_reset;
} decode_vals;

int LoadServices();
void *start_scanthread(void *);

/**************************************************************/
/*  functions for new command handling via CZapitClient       */
/*  these functions should be encapsulated in a class CZapit  */
/**************************************************************/

void addChannelToBouquet(unsigned int bouquet, unsigned int onid_sid);
void removeChannelFromBouquet(unsigned int bouquet, unsigned int onid_sid);
void sendBouquets(bool emptyBouquetsToo);
void internalSendChannels(ChannelList* channels);
void sendBouquetChannels(unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels(CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
void startPlayBack();
void stopPlayBack();
unsigned int zapTo(unsigned int channel);
unsigned int zapTo(unsigned int bouquet, unsigned int channel);
unsigned int zapTo_ServiceID(unsigned int serviceID, bool isSubService);
void sendAPIDs();

#endif /* __zapit_h__ */
