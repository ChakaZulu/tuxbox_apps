/*
 * $Id: zapit.h,v 1.60 2002/09/18 23:02:16 thegoodguy Exp $
 */

#ifndef __zapit_h__
#define __zapit_h__

#include <clientlib/zapitclient.h>

#include "bouquets.h"

typedef struct decode_struct
{
	bool new_tp;

} decode_vals;

int prepare_channels();
void save_settings (bool write);
void *start_scanthread(void *);
int start_scan();

/**************************************************************/
/*  functions for new command handling via CZapitClient       */
/*  these functions should be encapsulated in a class CZapit  */
/**************************************************************/

void addChannelToBouquet (unsigned int bouquet, t_channel_id channel_id);
void sendBouquets (bool emptyBouquetsToo);
void internalSendChannels (ChannelList* channels);
void sendBouquetChannels (unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels (CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
int startPlayBack ();
int stopPlayBack ();
unsigned int zapTo(const unsigned int channel);
unsigned int zapTo(unsigned int bouquet, unsigned int channel);
unsigned int zapTo_ChannelID(t_channel_id channel_id, bool isSubService);
void sendAPIDs ();

#endif /* __zapit_h__ */
