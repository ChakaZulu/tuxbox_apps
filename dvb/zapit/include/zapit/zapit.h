/*
 * $Id: zapit.h,v 1.55 2002/04/22 19:32:21 obi Exp $
 */

#ifndef __zapit_h__
#define __zapit_h__

#include "bouquets.h"
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
int startPlayBack();
int stopPlayBack();
unsigned int zapTo(unsigned int channel);
unsigned int zapTo(unsigned int bouquet, unsigned int channel);
unsigned int zapTo_ServiceID(unsigned int serviceID, bool isSubService);
void sendAPIDs();

#endif /* __zapit_h__ */
