/*
 * $Id: zapit.h,v 1.62 2002/09/25 18:51:13 thegoodguy Exp $
 */

#ifndef __zapit_h__
#define __zapit_h__

#include <zapitclient.h>

#include "bouquets.h"

void save_settings (bool write);
void *start_scanthread(void *);
int start_scan();

/**************************************************************/
/*  functions for new command handling via CZapitClient       */
/*  these functions should be encapsulated in a class CZapit  */
/**************************************************************/

void addChannelToBouquet (const unsigned int bouquet, const t_channel_id channel_id);
void sendBouquets (bool emptyBouquetsToo);
void internalSendChannels (ChannelList* channels);
void sendBouquetChannels (unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels (CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
int startPlayBack ();
int stopPlayBack ();
unsigned int zapTo(const unsigned int channel);
unsigned int zapTo(const unsigned int bouquet, const unsigned int channel);
unsigned int zapTo_ChannelID(const t_channel_id channel_id, const bool isSubService);
void sendAPIDs ();

#endif /* __zapit_h__ */
