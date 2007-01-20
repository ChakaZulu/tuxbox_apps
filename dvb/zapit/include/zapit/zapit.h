/*
 * $Id: zapit.h,v 1.70 2007/01/20 20:12:37 houdini Exp $
 */

#ifndef __zapit_h__
#define __zapit_h__

#include "client/zapitclient.h"

#include "bouquets.h"

void save_settings (bool write);
void *start_scanthread(void *);
int start_scan(bool scan_mode);

/**************************************************************/
/*  functions for new command handling via CZapitClient       */
/*  these functions should be encapsulated in a class CZapit  */
/**************************************************************/

void addChannelToBouquet (const unsigned int bouquet, const t_channel_id channel_id);
void sendBouquets        (int connfd, const bool emptyBouquetsToo, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void internalSendChannels(int connfd, ChannelList* channels);
void sendBouquetChannels (int connfd, const unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels        (int connfd, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, const CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
int startPlayBack(CZapitChannel *);
int stopPlayBack(void);
unsigned int zapTo(const unsigned int channel);
unsigned int zapTo(const unsigned int bouquet, const unsigned int channel);
unsigned int zapTo_ChannelID(const t_channel_id channel_id, const bool isSubService);
void sendAPIDs(int connfd);
void enterStandby(void);
void leaveStandby(void);
void setVideoSystem_t(int video_system);
void setIec(int);
void setDemuxMode(int);

#define PAL	0
#define NTSC	1

#endif /* __zapit_h__ */
