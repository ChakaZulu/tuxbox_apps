/*
 * $Id: zapit.h,v 1.65 2002/12/20 19:19:46 obi Exp $
 */

#ifndef __zapit_h__
#define __zapit_h__

#include "client/zapitclient.h"

#include "bouquets.h"

void save_settings (bool write);
void *start_scanthread(void *);
int start_scan();

/**************************************************************/
/*  functions for new command handling via CZapitClient       */
/*  these functions should be encapsulated in a class CZapit  */
/**************************************************************/

void addChannelToBouquet (const unsigned int bouquet, const t_channel_id channel_id);
void sendBouquets        (int connfd, const bool emptyBouquetsToo);
void internalSendChannels(int connfd, ChannelList* channels);
void sendBouquetChannels (int connfd, const unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels        (int connfd, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, const CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
int startPlayBack ();
int stopPlayBack ();
unsigned int zapTo(const unsigned int channel);
unsigned int zapTo(const unsigned int bouquet, const unsigned int channel);
unsigned int zapTo_ChannelID(const t_channel_id channel_id, const bool isSubService);
void sendAPIDs(int connfd);
void enterStandby(void);
void leaveStandby(void);

#endif /* __zapit_h__ */
