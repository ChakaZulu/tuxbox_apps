/*
 * $Id: zapit.h,v 1.58 2002/05/12 01:56:18 obi Exp $
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

void addChannelToBouquet (unsigned int bouquet, unsigned int onid_sid);
void removeChannelFromBouquet (unsigned int bouquet, unsigned int onid_sid);
void sendBouquets (bool emptyBouquetsToo);
void internalSendChannels (ChannelList* channels);
void sendBouquetChannels (unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void sendChannels (CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);
int startPlayBack ();
int stopPlayBack ();
unsigned int zapTo (unsigned int channel);
unsigned int zapTo (unsigned int bouquet, unsigned int channel);
unsigned int zapTo_Onid_Sid (unsigned int serviceID, bool isSubService);
void sendAPIDs ();

#endif /* __zapit_h__ */
