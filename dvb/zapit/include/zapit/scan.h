/*
 *  $Id: scan.h,v 1.27 2005/01/12 19:38:13 thegoodguy Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#include <linux/dvb/frontend.h>

#include <inttypes.h>

#include <map>
#include <string>

#include "bouquets.h"

#include "getservices.h"

extern std::map <scantransponder_id_t, transponder> scantransponders;
typedef std::map <scantransponder_id_t, transponder>::iterator stiterator;
#define GET_ORIGINAL_NETWORK_ID_FROM_SCANTRANSPONDER_ID(transponder_id) ((t_original_network_id)(transponder_id      ))
#define GET_TRANSPORT_STREAM_ID_FROM_SCANTRANSPONDER_ID(transponder_id) ((t_transport_stream_id)(transponder_id >> 16))

extern CBouquetManager* scanBouquetManager;

char * getFrontendName();

#endif /* __scan_h__ */
