/*
 * $Id: zapit.h,v 1.53 2002/04/20 12:02:04 Simplex Exp $
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
#include <sys/wait.h>

#include "bouquets.h"
#include "cam.h"
#include "cat.h"
#include "frontend.h"
#include "getservices.h"
#include "pat.h"
#include "pmt.h"

#define AUDIO_DEV "/dev/ost/audio0"
#define FRONT_DEV "/dev/ost/frontend0"
#define VIDEO_DEV "/dev/ost/video0"

#include "zapitclient.h"

typedef struct decode_struct
{
	bool new_tp;

} decode_vals;

int prepare_channels();
int save_settings (bool write);
void *start_scanthread(void *);
int start_scan();
void sendBouquetList();
void sendChannelListOfBouquet( uint nBouquet);

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
